> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Real backend runtime diagnostic 0.5-runtime.1

Worker-only LiveArea persistence passed by user report through sample 60, with folders/apps normal. The next gate is actual backend startup and shutdown, not another closure test or music playback.

This private diagnostic uses the exact real backend archive from home.1 (SHA-256 71a7f505f9087e90b5b39b703251c9f4be6270771a9449621328e1ab018de5f0) and SDK snapshot sdk-snapshot-20260825.611.1. It invokes vam_init, drains/releases events, samples numeric auth state and malloc statistics for at most fifteen two-second intervals, then invokes vam_shutdown. It requests a 48 MiB worker SELF with 24 MiB newlib heap; these settings are not yet device-validated. Parent remains 4 MiB/256 KiB. Worker SFO retains the successful 0x04008000, parent stays 0x8000; both install versions advance to 01.06.

The 4 MiB diagnostic heap was never a playback budget. Source inspection shows a 1 MiB network pool, a runtime thread with a requested 1 MiB stack, two Tokio workers with requested 512 KiB stacks, plus additional runtime/network working sets. This trial chooses more headroom; it does not establish a maximum permitted gdd budget or optimal memory configuration. mallinfo snapshots cover that allocator, not all process memory or the exact peak between samples.

Each run creates a fresh ux0:data/VitaSpot-ServiceProbe/runtime-<PID>/ directory and refuses reuse. It does not read ux0:data/VitaSpot/ or copy the user's credentials. Network initialization and local discovery are exercised. The device name is VitaSpot Runtime Probe - DO NOT CONNECT. Do not pair or select it in Spotify. Audio-port opening and BGM acquisition are deliberately blocked by link wrappers that return -1; native checks verify both real acquisition symbols are absent and both wrappers return the denial without making calls. This is not a working playback package.

Before initialization a separate thread is started for a nominal 60-second watchdog. It writes runtime-watchdog.log and exits only the probe process if the normal main path has not completed. This is diagnostic fallback, not a successful shutdown or a hard real-time guarantee. Abort, scheduler, I/O and loader failures can still prevent full logging. Worker event draining is bounded to 32 per observation; every event taken is released in the worker. Logs requested below contain numeric status/memory fields, not account details.

Native cross-build and three package/evidence tests pass: exact SELF/SFO pairing/versions, compiled zero launch argument/lifecycle calls/24 MiB heap/audio denial, and frozen source/resource hashes. Backend initialization itself has not run on this host or device. Original earlier artifacts and device evidence remain unchanged.

## Test once

1. Close VitaSpot manually. Keep Wi-Fi enabled. Install the VPK over the separate Service Probe bubble and launch once; launcher.log should identify probe=5.
2. Leave it running at Home with the Vita awake for about 75 seconds. Do not close its LiveArea page during this check. Do not select the probe in Spotify, launch games or test power/sleep.
3. Send ux0:data/VitaSpot-ServiceProbe/launcher.log and runtime.log. If runtime-watchdog.log exists, send it too. Do not send the backend's vam.log or credentials files. Old heartbeat.log and memory.log are unrelated.
4. Report whether Home folders and an ordinary app work normally afterward. If startup fails or a log is missing, report the existing files; do not repeat launch.

Expected normal result: before_init, after_init code=0, runtime samples, before_shutdown, after_shutdown and status=stopped reason=runtime_probe init_result=0 log_failed=0 guard_wait=0. A zero init return alone does not prove the asynchronous runtime reached logged-out/discovery-ready state; inspect the numeric auth samples as well. A watchdog timeout is a failed startup/shutdown diagnostic, not a pass.

SHA-256: 47c7c985a0663f64cb39fe5bd979f26317e134b53c70ed676b5140706683fe44.
ELFs/VELFs are in build/service-runtime-probe/. Frozen sources, disassembly, verification.json and native build log accompany the VPK.

## Device acceptance, 11 November 2026

Launcher load/start=0. Worker PID 668689 reports init_result=0; auth samples transition from starting (0) to logged_out (1). Filtered backend diagnostics confirm discovery advertising, engine stop, runtime stop and thread join. All 15 observations complete; after_shutdown appears, followed by status=stopped reason=runtime_probe init_result=0 log_failed=0 guard_wait=0. The user confirms normal folders and ordinary app launches afterward. Startup/discovery/shutdown pass with audio deliberately blocked.

Maximum sampled malloc use was 1,084,504 bytes (approximately 1.03 MiB), falling to 8,040 bytes after shutdown; shutdown-stage timestamps span 0.8167 seconds. These are observations of this idle allocator, not total residency, exact high water or a playback memory budget. The arena was 1,155,072 bytes; its 70,568 bytes of free chunks during idle do not mean the entire configured 24 MiB heap was almost exhausted. Unused capacity can remain outside the currently grown arena. Thread stacks and other kernel allocations are separate. Numerical logs, original hashes and selected non-account backend diagnostics are in artifacts/service-probes/device-evidence/probe05-runtime1/. Raw backend vam.log is not copied.

The next diagnostic enables actual service playback, preserving the startup-tested 48 MiB SELF request/24 MiB heap and worker persistence bit. It uses fresh test credentials and a controlled pause/shutdown; no game or power transition is included. See service-playback.md.
