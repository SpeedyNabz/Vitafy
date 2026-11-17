> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Bounded Spotify service playback 0.6-playback.1

Runtime Probe 0.5 passed real initialization, logged-out discovery, normal shutdown and post-stop shell health. This next private VPK tests real Spotify audio in the separate background worker. It is not the finalized VitaSpot frontend or a game-playback release.

## Implementation

Uses the exact home.1 Rust archive and SDK snapshot sdk-snapshot-20260825.611.1. Worker keeps the startup-tested 48 MiB SELF request, 24 MiB heap and ATTRIBUTE=0x04008000. Parent remains 4 MiB/256 KiB with ATTRIBUTE=0x8000. Both APP_VER values advance to 01.07, launcher identifies probe=6. The previous audio-denial wrappers are removed; native checks verify real BGM acquisition and audio-port imports, without those wrappers.

Each run creates a new ux0:data/VitaSpot-ServiceProbe/playback-<PID>/ directory; it refuses reuse and never accesses ux0:data/VitaSpot/. Pairing stores the test credentials only there. The device appears as VitaSpot Service Test in Spotify Connect. Initial requested volume is 50%; adjust cautiously with the Vita/phone controls. Bitrate is 96 kbps and the disk audio-cache cap is 1 MiB; neither is a memory-capacity claim.

The worker polls/releases bounded event batches and logs numeric auth/playback state, position and allocator/kernel-pool snapshots every two seconds. No titles, URIs, usernames or tokens are written by this diagnostic driver. Normal observation is at most 120 two-second waits (roughly four minutes from init), then it requests pause and calls vam_shutdown, which should release audio ownership and join threads. Old heartbeat.log, memory.log and runtime.log belong to earlier tests.

A separate monitor records playback-timeout.log after a nominal five minutes if normal completion is missing. It reports only and does not force-terminate the audio owner. Such a report is a failure to reach normal completion, not a safe-shutdown proof. Neither loop is a hard real-time watchdog. Native checks verify the monitor contains no call to sceKernelExitProcess. A stop.request file in the probe directory can request early shutdown while the main polling loop remains responsive; the launcher clears a previous request.

Native cross-build and three package/evidence tests pass: actual SELF/SFO/CRC/pairing, compiled lifecycle/player/event calls and real heap/audio import/timeout behavior, and frozen sources/resource hashes. Device authentication, actual audio output, memory during decoding and teardown with active music remain pending.

## Test once, at Home only

1. Manually close VitaSpot. Install over the separate Service Probe bubble. Put the Vita and phone on the same Wi-Fi; launch once.
2. In the phone's Spotify app, select VitaSpot Service Test from the device picker and start a track. Confirm sound comes from the Vita/headphones, rather than continuing on the phone. No password or token needs to be shared.
3. Stay at Home with the Vita awake. After about 30 seconds of audio, skip to another track from Spotify and confirm playback continues. Do not close/remove the probe LiveArea page during this run, launch a game, or press power. Keep this focused on audio startup/decoding/shutdown.
4. The test should stop music automatically around four minutes after initialization (the pairing time counts). Check that Home folders and an ordinary app still work normally afterward.
5. Send ux0:data/VitaSpot-ServiceProbe/launcher.log and playback.log. If playback-timeout.log exists, send it too. Report whether pairing, Vita audio, track skip and automatic stopping worked. Do not send credentials.json or the backend's vam.log; request-specific diagnostics can be filtered locally if needed.

If the device never appears, sound fails, playback ends early, or automatic stopping fails, report the current logs and observations without repeated launches or a game-triggered close. A numeric playing state alone does not prove audible output. Sampled allocator use is not exact peak or total residency. Game compatibility, active-frontend force-close safety, reconnection and power/sleep remain unverified.

VPK SHA-256: de219ff517b9ed2ca154e61a8dc41f487d97bf8316be23fb59a92074251e96f3.
ELFs/VELFs are in build/service-playback-probe/. Frozen sources, disassembly, build log and verification.json accompany the artifact.
