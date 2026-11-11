# Architecture and notifications

Vita input → C/SDL2 frontend → C ABI version 3 → Rust/librespot → Vita network
and `sceAudioOut` output. Music is decoded on the Vita; this is not a remote
controller or an executable wrapper. Desktop development substitutes only the
backend with a synthetic catalogue.

The frontend polls bounded event batches and releases backend-owned objects with
`vam_event_release`. Network work stays on backend threads. UI snapshots, images,
navigation and persistent preferences remain separate from the audio sink.
The Vita adapter contains SDK-specific lifecycle, power and notification calls.

## Notifications

`src/notifications.c` contains portable track-change detection and bounded UTF-8
to UTF-16 conversion. It sends only when this device is active and a nonempty
track is actually in the playing state. It suppresses duplicate snapshots and
coalesces rapid track skips over five seconds; only the latest playing track is
considered after that interval. Pause/progress changes do not create new notices.

The platform adapter loads `SCE_SYSMODULE_NOTIFICATION_UTIL`, links
`SceNotificationUtil_stub` and passes the SDK-required 0x410-byte UTF-16 buffer to
`sceNotificationUtilSendNotification`. Normal applications do not need
`sceNotificationUtilBgAppInitialize`. The installed SDK header and native import
checks are the authority; hardware delivery is still pending. Reference:
[VitaSDK notification declarations](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/notificationutil.h).

The On/Off preference controls future sends. Failures produce numeric diagnostics
and do not retry every frame; the frontend also shows a song/artist toast. No
system notification history is deleted. This is a track-change notification,
not a continuously updated shell media widget; historical entries can show old
tracks and pausing does not rewrite them.

## Lifecycle boundaries

The inspected `_sceAppMgrGetAppState` and controller-interception state gate
graphics/input while the system overlays the application. Audio threads remain
independent. This is sampled state, not an atomic GPU ownership guarantee.
The app uses normal package attributes and offers orderly shutdown in Settings.

A separate service would need a bounded/versioned IPC protocol, serialization,
request IDs, reconnect/resynchronization and ownership retained in the worker.
Raw backend pointers cannot be passed between processes. Idle service startup,
LiveArea persistence and real streaming are different milestones. See the
[research index](research-index.md) for the device experiment ledger.
