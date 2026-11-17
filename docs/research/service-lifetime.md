> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Separate player lifetime: bounded worker preflight

**Latest device result:** Service Probe 0.2 returned start_bg_app=0; the user confirms heartbeat samples keep increasing after the launcher returns to Home. The supplied log now confirms samples 0 through 60 under worker PID 9055701 and a limit-stop record after 120.31 seconds. The user confirms post-stop shell health is normal. Independent process exit is not measured. The [real-backend static audit](service-memory.md) now shows 15.16 MiB of load segments before runtime allocations. See [current evidence and next check](service-launch-result.md).

**Historical launch investigation:** 0.1's module load succeeded but start returned 0x80106501, and no heartbeat file appeared. Its argument 1 follows a misleading SDK comment. The separately versioned [0.2 correction](service-launch-result.md) uses the native sample's argument 0 with the worker otherwise byte-identical. Install/test 0.2 for the next check; the original 0.1 procedure below is historical.

9 November 2026. The latest user observations narrow the problem:

- On home.1, music was playing, the user went to Home, closed VitaSpot directly from Home, then reopened it successfully. Returning to the foreground before closing was not required for that successful case.
- A game's forced-close prompt while home.1 plays still hangs the shell and requires a forced reset.
- Disabling AppMgr BGM ownership in home.2 stops Home audio, while returning and ordinary closing still work. Removing that ownership loses the desired behavior and is not being promoted.

These results do not support a generic claim that closing from Home is broken, or prove that BGM ownership itself causes the game-close hang. The game-launch transition differs from the successful manual close. No repeat of the failing transition is requested.

## Restore the Home-working build

home.1-restored (historical lab reference: `../artifacts/experiments/VitaSpot-0.0.5-home.1-restored/VitaSpot-0.0.5-home.1-restored.vpk`) has the exact home.1 executable and resources with only the install version raised to 01.12. This allows installation over home.2 without a downgrade. It still identifies itself as home.1 inside the app and retains the known game force-close failure. Preserve ux0:data/VitaSpot/.

For Home listening, close VitaSpot manually before starting a game; do not accept a game-triggered close prompt while it is playing. recovery.5 (historical lab reference: `../artifacts/recovery/VitaSpot-0.0.4-recovery.5/VitaSpot-0.0.4-recovery.5.vpk`), install version 01.13, restores the original working foreground-only executable if needed. Neither update claims a new game-close fix.

## Why test a separate worker

A player that must be killed to launch a game cannot continue streaming in that process. A separate userland background application could own the network/decoder/audio while the frontend closes independently. Before moving the comparatively large Rust backend, this preflight tests only the launch/lifetime mechanism. It is not a Spotify service implementation or proof of memory/audio compatibility alongside a game.

VitaSpot-ServiceProbe-0.1.vpk (historical lab reference: `../artifacts/service-probes/VitaSpot-ServiceProbe-0.1/VitaSpot-ServiceProbe-0.1.vpk`) installs as **VitaSpot Service Probe**, title VTSV01001, with a paired VTSV01002 background executable. It does not replace VTAM00001 or read/write ux0:data/VitaSpot/. It uses only ux0:data/VitaSpot-ServiceProbe/.

The launcher loads the installed SDK's BG_APP_UTIL module, calls sceBgAppUtilStartBgApp(1), writes its result, and returns from main. There is no on-screen UI; a brief launch screen followed by Home is expected. The worker writes a heartbeat every two seconds for at most 60 waits, then returns. A stop.request file in its own directory can request an earlier stop. Each run replaces the probe's own logs and the launcher clears a prior stop request. Do not launch it repeatedly while the first worker is still active.

The worker initializes no networking or audio, links no Spotify/SDL/GXM code, changes no clocks or power policy, and uses no shell/kernel plugin. Native newlib file I/O links dormant socket send/receive/close dispatch helpers; there is no network initialization/socket creation/connect API in either executable. No account credentials are involved.

## First device check: launcher returns, worker continues

1. Close VitaSpot manually first so it is not an active player during this check. Do not use a game's close prompt.
2. Install the separate Service Probe VPK with VitaShell and launch its bubble once. It should return to Home automatically; it does not play music.
3. After roughly 15 seconds, inspect ux0:data/VitaSpot-ServiceProbe/launcher.log and heartbeat.log with your existing VitaShell/file-transfer method. Record start_bg_app's result and the last heartbeat's sample/PID.
4. Close and reopen the log after another 15–20 seconds. A higher sample number with the same worker PID, after the launcher returned, supports independent worker execution.
5. After about two minutes awake, the final line should say status=stopped reason=limit, normally sample=60. The loop is bounded by 60 waits, not a hard real-time watchdog; scheduling, suspension or I/O delays can extend elapsed time.

Do not launch a game, peel the probe's LiveArea bubble, test power/sleep or leave it running as a player in this first check. Its initial parent flags do not request persistence across explicit LiveArea removal; that is a separate lifecycle requirement. If startup or shell behavior is abnormal, stop and report the existing log/result rather than repeating the launch.

This check is successful only if the launcher returns, the worker advances afterward, and it stops cleanly. A zero launch result without fresh/advancing heartbeat data is insufficient. No heartbeat may indicate rejected boot metadata, early worker exit, permission/I/O problems or parent/worker coupling; the recorded result helps narrow it.

After the worker has stopped, the separate bubble can be uninstalled without affecting VitaSpot. The original application/account folder remains separate.

## Implementation and verification

Sources are in service-probe/. The standalone launcher is category gdc; its worker is gdd, packaged as eboot2.bin with sce_sys/param2.sfo. PUBTOOLINFO=has_bg=1 and the child install-directory references pair the two titles. Both have 4 MiB SELF budgets and explicit 256 KiB newlib heaps. Only the worker gets SELF Control Info ATTRIBUTE=3. SFO ATTRIBUTE remains the minimal original communication-zone value 32768; system-suspend disabling, no-forced-kill and background-survival-on-peel flags were not added.

The installed SDK supports these SELF fields directly, so no DolceSDK installation or guessed API/NID was needed. BG_APP_UTIL uses the installed weak stub library because the launcher loads that module in main, following the reference sample's late-import pattern. The framebuffer warning in the reference sample is avoided by using no graphics in this preflight. Probe 0.2 device evidence now confirms accepted launch and advancing worker execution after parent return. The supplied device log confirms the limit-stop path and the user confirms normal post-stop shell behavior. Explicit frontend closure remains pending.

The host C contract passed bounded lifetime, requested stop, immediate stop, write failure and invalid-input cases. Native compilation passed. Package checks verify the two titles/classes, parent-child metadata, boot fields, each executable's strong heap override and exact packaged SELF payload. Three binary/package tests also reject a missing or substituted worker and verify source/ELF hashes. Native text/data/BSS totals are approximately 330 KiB per executable, plus the 256 KiB heap, within their requested 4 MiB budgets. These are build observations, not measured device residency or power use.

The VPK reuses only the existing private-test VitaSpot icon/LiveArea files; source pin and resource hashes were verified. It remains a private preflight, with no public distribution of upstream assets.

`py scripts/build_service_probe.py` builds/tests the host contract, builds the two native executables, validates the package and refuses to overwrite a previous probe directory. `py -m unittest discover -s tests -p test_service_probe.py` checks the real generated artifacts. Logs, ELFs/VELFs, verification.json and SHA256SUMS are retained locally.

Service Probe VPK SHA-256: d2c0fca95d6466594d97b36d2d323dd6b1a034f2f2a40a90e4668043f2b54381. Restored Home VPK SHA-256: 95d307945ebf7ad353a0786d217a01c07590fb88d4af634118b612442046fafb. Foreground-only recovery.5 SHA-256: 1f0dacecd415a0582cb076995a1a51d2c1df5ad3a9ce3f6678427f460c36a220.

## Work still needed after lifetime proof

- Establish persistence across explicit frontend closure and then resource coexistence without accepting a forced close.
- Measure the background memory limit against the actual Rust backend; this small preflight does not establish that the player fits.
- Move ownership of session/decoder/BGM to the worker, with explicit start/stop and recovery behavior.
- Add bounded IPC for the frontend. C ABI pointers and ownership cannot be shared directly across processes.
- Validate game compatibility, Wi-Fi interruptions, UI reconnection and final shutdown. Keep true hardware suspend distinct from display-off playback.

## Primary references

- Installed VitaSDK psp2/bgapputil.h declares sceBgAppUtilStartBgApp and mode 1; sysmodule.h supplies BG_APP_UTIL. [Upstream header](https://github.com/vitasdk/vita-headers/blob/e66ebe90b73d1fa4cce005a5b2072cec27322544/include/psp2/bgapputil.h).
- [GrapheneCt BG-App-PSV CMake](https://github.com/GrapheneCt/BG-App-PSV/blob/49dbde8ae9ac8fc2adbd5884a0fa9313046bac84/CMakeLists.txt) and [README](https://github.com/GrapheneCt/BG-App-PSV/blob/49dbde8ae9ac8fc2adbd5884a0fa9313046bac84/README.md) establish the public paired-application pattern. Its older raw call uses 0; this probe follows the installed SDK declaration. Notification/keep-awake flags and source code were not copied.
- [VitaSDK SELF generator](https://github.com/vitasdk/vita-toolchain/blob/008ddc2d7fd4bd16631d1e49cacf308de6f545f1/src/vita-make-fself/vita-make-fself.c) and installed vita.cmake document KiB memory budgets and the separate control attribute.
