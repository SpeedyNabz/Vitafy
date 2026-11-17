# Current research findings

Updated 17 November 2026. This synthesis connects the archived experiments to the
current application. Read [status](../status.md) and the
[open work plan](../open-work-plan.md) for the active backlog. Historical experiment
instructions remain historical; this page does not request new device actions.

## Reproducible baseline

Current development source is 0.1.1-preview.1 / ABI v6. Released 0.1.0 is the
unchanged user-accepted preview.7 Home 01.29 binary. Development Home is 01.31;
its foreground build configuration is 01.32. Source pins remain VitaSpot
`81c533f1ef61623360ea4fbe32cf29b4f2f3901f`, VitaSDK
`sdk-snapshot-20260825.611.1`, GCC 15.2.0, Rust nightly-2026-10-18.
No SDK or upstream upgrade was made for these findings.

## New app findings

The user reports device/Bluetooth volume alternation and long buffering after
Next. Source inspection found generic system-volume polling every 50 ms, volume
writeback through both the engine and Connect, and preloading gated on the last
30 seconds plus the full remaining download. These are plausible contributing
mechanisms, not measured Bluetooth root-cause proof.

The candidate separates app/Connect gain from physical output levels, removes
generic system-volume imports, labels App volume and accepts mute snapshots.
It preloads one next song after two seconds with approximately three seconds of
buffered headroom (or complete current data), updates changed queue targets,
cancels superseded async loading and filters old request events. Speculative
failure no longer marks the queued song unavailable. The async 45-second deadline
cannot interrupt arbitrary blocking decoder/file/transport calls.

32 Rust tests, nine desktop tests, five UI suites and native package checks pass;
native symbols confirm generic volume Get/Set are absent and debug sections remain.
Bluetooth behavior and skip latency are unmeasured on this candidate. See the
[full investigation](../bluetooth-volume-and-skip-buffering.md),
[user report](../evidence/bluetooth-skip-user-report.json),
[host checks](../evidence/bluetooth-skip-host.json) and
[native checks](../evidence/bluetooth-skip-home-native.json).

Preview.7 already implemented account-scoped metadata persistence, immutable
playlist snapshot reuse, bounded LRU/budgets, freshness and root/saved refresh
separation. Sorted playback supports 10,000 playable entries with bounded chunk
upload and explicit fallback; the existing Connect queues roll within 80 next /
10 previous entries. The full order still occupies memory proportional to its
size. Physical performance/cache/large-order soak measurements remain open.
See [implementation bounds](../releases/preview7.md).

## What the service experiments actually established

| Historical observation | Interpretation and limit |
| --- | --- |
| Start argument 1 failed; argument 0 launched the paired worker | The installed header says 1, but the tested sample/device required 0. Preserve this contradiction and validate actual behavior; neither prose nor a zero return alone proves lifetime. |
| Unflagged and parent-only persistence controls stopped after page removal | Parent-side changes did not solve that specific test. Exact termination cause was not measured. |
| Worker-only ATTRIBUTE 0x04008000 continued to sample 60 by user report | A tiny no-audio heartbeat survived normal LiveArea removal on that device. The retained raw log covers early samples only. This is not active-UI kill, audio or game evidence. |
| Runtime 0.5 initialized, advertised discovery and shut down with audio blocked | Historical backend startup/idle cleanup passed; maximum sampled malloc was 1,084,504 bytes and observed shutdown span 0.8167 s. These are not total/peak playback residency or a current performance promise. |
| Historical backend ELF PT_LOAD totaled 15,893,348 bytes (15.16 MiB) | Already exceeded the tiny probe's 4 MiB request before runtime allocations. It measured an older archive; current ABI v6 and caches/preload must be audited again. |
| Worker requested 48 MiB SELF / 24 MiB heap for runtime/playback probes | Idle startup was observed with that request. It does not establish maximum granted budget, decoding headroom or game coexistence. |
| 0.6 real-audio worker package was built | No real-audio acceptance response is recorded. A build or numeric playing state cannot close this gap. |

Sources: [launch/lifetime](service-lifetime.md),
[parent control](service-close-parent.md), [worker control](service-close-worker.md),
[idle runtime](service-runtime.md), [memory audit](service-memory.md),
[pending real audio](service-playback.md). Probe source/build tooling remains in
the historical lab rather than a completed standalone production service.

## Implications for playback across apps and games

The current main app owns its session, decoder, audio port and BGM lease in its
own process. A terminated process cannot continue playback. Home continuation
therefore proves a different capability from continuing after another title
requires that process to close. Removing BGM acquisition stopped Home audio in the
historical control; that does not prove BGM caused the game-close shell hang.
Reordering ordinary destructors cannot guarantee cleanup on forced termination.

A separate userland worker is the preferred candidate, keeping playback ownership
outside the UI process. Its required evidence is current-budget feasibility,
real audio/clean stop, active-audio lifetime, bounded IPC/reconnect, contention
policy and a tested app/game matrix. No inspected declaration grants universal
coexistence. Installed `psp2/appmgr.h` declares acquire/release and a BGM-busy
error; it does not promise another title will leave resources available.

The known game-triggered forced-close transition remains excluded. Do not repeat
it or infer that a persistence bit fixes it. Plugin/system integration, if userland
proves insufficient, is a separate research option with additional compatibility
and recovery requirements. No such integration is built or installed.

Recorded hardware is PCH-1003; firmware/plugin inventory is still missing. Cross-model
and firmware coverage cannot be inferred. Display-off, power-button behavior,
true suspend, continuous Wi-Fi/audio and resume recovery remain distinct unknowns.
For the ordered dependencies and completion evidence, use
[the open work plan](../open-work-plan.md).
