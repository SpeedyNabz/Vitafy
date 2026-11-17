# Architecture and notifications

Updated 17 November 2026 for development 0.1.1-preview.1 / ABI v6. Released 0.1.0
is the accepted preview.7 binary; see [status](status.md). Source and SDK pins are
in [source-lock.json](../source-lock.json) and [building](building.md).

## Current single-process player

Vita input -> C/SDL2 frontend -> process-local C ABI v6 -> Rust/librespot ->
Vita network and `sceAudioOut`. Music is decoded locally on the Vita. The desktop
preview substitutes a synthetic backend and produces no Spotify audio.

The engine owns the Spotify session, Connect task, mixer/player and platform sink.
The sink owns the BGM lease, audio port, ring and output thread. The frontend
polls bounded event batches and releases backend-owned objects using
`vam_event_release`. Network/metadata/IO work stays off the rendering thread.
This pointer ownership is valid within a process; it is not an IPC protocol.

## Metadata and ordered playback

Account/session-scoped playlist snapshots serve consistent 50-row slices. Compact
track/library/playlist disk records provide labelled stale-first browsing;
background refresh, epochs and revision checks prevent stale edits or old-account
publication. Authoritative hearts remain separate from disk browse summaries.
Byte/count budgets and incremental LRU bound retention; settings expose 16/32 MiB
browse storage and an acknowledged clear action.

The frontend retains order indices and sends up to 128 playable IDs per frame
through begin/append/commit/cancel. The backend caps the order at 10,000 playable
entries / 1 MiB URI bookkeeping. Duplicate entries retain positions. The full
ordered context remains proportional to list length, while Connect rolls its next/
previous queues within 80/10. Preparation failures expose an explicit original-order
fallback. See [cache and order bounds](releases/preview7.md); hardware stress and
cold/warm performance are still open.

## Volume and preload candidate

In 0.1.1-preview.1, Spotify/app gain uses a software mixer independently of Vita
speaker/headset hardware levels. No generic system-volume polling/writeback is
performed. Connect alone applies queued slider changes; mute is valid. Physical
buttons therefore do not move the App volume slider. Source quality remains Best
(lossless first), Medium <=160 kbps or Low <=96 kbps; Vita output is S16 PCM.

One next-song loader may be requested after two seconds when approximately three
seconds are buffered ahead, or current data is complete; short/end-of-track behavior
remains. Queue/context changes refresh the target. Superseded async loads are
cancelled, speculative failures retain queue entries, and stale request events are
ignored. The 45-second async deadline cannot interrupt arbitrary blocking IO.
These changes have host/native evidence; device routing and latency remain pending.
See [the investigation](bluetooth-volume-and-skip-buffering.md).

## Notifications

`src/notifications.c` provides portable track-change detection and bounded UTF-8
to UTF-16 conversion. It sends only for this active device and a nonempty track in
the playing state, suppresses duplicates and coalesces rapid skips over five
seconds. Pause/progress do not generate new notices. This is not a continuous
shell media widget and does not rewrite old notification history.

The adapter loads `SCE_SYSMODULE_NOTIFICATION_UTIL`, links
`SceNotificationUtil_stub` and passes the SDK-required 0x410-byte UTF-16 buffer to
`sceNotificationUtilSendNotification`. It does not initialize a background
notification service. SDK declarations/native imports support this implementation;
full device delivery/Unicode acceptance is still a separate check. Failures produce
numeric diagnostics without retrying every frame; the preference controls future
sends and does not delete notification history.

## Lifecycle and proposed service boundary

Sampled AppMgr/controller-interception state gates frontend graphics/input while
system overlays are active. Rendering, GPU uploads and frontend display policy
are gated separately from audio. This is not atomic GPU ownership proof. The
foreground build has ordinary package attributes; the released/test Home profile
uses ATTRIBUTE=32776. The known game-triggered forced-close hang remains.

| Concern | Current app | Proposed worker integration, not implemented |
| --- | --- | --- |
| Playback lifetime | Audio owner is inside the graphical application's process | Paired minimal userland worker owns session/network/decoder/audio |
| UI closure | Normal Settings Exit stops the player; forced death cannot guarantee destructors | Detach preserves worker playback; Stop is a distinct acknowledged command |
| Data exchange | Process-local ABI pointers/events | Bounded versioned serialized values, IDs/generations, reconnect snapshots and backpressure |
| Credentials/events | Owned and released in current backend | Owned/released in worker; never transferred as raw pointers or exposed in logs |
| Background memory | Frontend and backend caches share process resources | Measured worker budget; trim UI-only metadata/artwork work when detached |
| Compatibility | Home reported working; games unproven/known unsafe close | Requires audio contention policy, real lifecycle/resource evidence and title matrix |

A worker is a candidate, not proof of all-game support. Historical heartbeat and
idle-backend tests used older sources and do not establish current active-audio
lifetime. The [current research synthesis](research/current-findings.md) records
those limits; [the open work plan](open-work-plan.md) orders the remaining gates.
