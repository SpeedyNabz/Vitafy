> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

# Service Probe 0.1 launch rejection and 0.2 correction

Device result, 9 November 2026 (user report):

```text
probe=2 pid=2436767 start_us=62618
load_bg_app_util=0
start_bg_app=0 launcher_returning=1
```

The heartbeat sample keeps increasing after the launcher returns to Home. This confirms accepted launch and continuing worker execution. The supplied launcher PID is 2436767. The subsequently supplied heartbeat.log contains all 61 samples (0 through 60) under worker PID 9055701, with strictly increasing timestamps and a final status=stopped reason=limit after 120.313727 seconds. This confirms the bounded stop path was reached; the user subsequently confirmed everything works correctly, completing the requested post-stop shell check. Independent process-exit confirmation remains unmeasured. The original log and parsed checks are retained in artifacts/service-probes/device-evidence/probe02-bounded-stop/. Explicit LiveArea removal, game coexistence, audio/Spotify, player memory capacity and power/sleep remain untested.

The initial service preflight is complete. The next gate is [actual-backend memory profiling](service-memory.md), followed by independent frontend-closure validation before migrating playback. No repeat of the initial probe, game or forced-close test is requested.

## Historical 0.1 failure

The user reports the launcher returns, with launcher.log present but no heartbeat.log. The supplied log is:

```text
probe=1 pid=23406355 start_us=88688
load_bg_app_util=0
start_bg_app=0x80106501 launcher_returning=1
```

The module loaded successfully. The background-start request was rejected; there is no evidence that this request launched the worker. This result must not be treated as a file-writing, timeout or worker-budget failure yet.

## Contradictory SDK comment

The installed VitaSDK bgapputil.h says the mode must be 1, and its error enum lists 0x80106502. Probe 0.1 followed that comment. However, [GrapheneCt's native caller](https://github.com/GrapheneCt/BG-App-PSV/blob/49dbde8ae9ac8fc2adbd5884a0fa9313046bac84/appmain/main.c) passes 0, and [its earlier error declaration](https://github.com/GrapheneCt/Vita-PoC/blob/8be840de04630b5fbe35c2828236c418d49ef19b/bgapputil.h) identifies 0x80106501 as invalid argument. An inspected [native implementation reference](https://github.com/shoui520/psp2/blob/e67caee7f38c70642b709459323789704ca89f60/sys/internal/usermode/src/bgapputil/bgapputil.cpp) rejects nonzero flags before making a shell launch request.

That reference is not a disassembly of the user's actual firmware. The observed return code and native sample justify a bounded zero-argument test. The SDK header itself is preserved; no toolchain files were modified.

## Current probe 0.2

VitaSpot-ServiceProbe-0.2.vpk (historical lab reference: `../artifacts/service-probes/VitaSpot-ServiceProbe-0.2/VitaSpot-ServiceProbe-0.2.vpk`) changes the launch argument from 1 to 0 and labels its launcher log probe=2. Its launcher package version is 01.01 so it can update the existing Service Probe bubble. The separate titles/data path remain unchanged.

The worker executable, worker param2.sfo, budgets, attributes, duration, stop behavior and every other bundled resource are byte-identical to 0.1. No start-handshake delay, flags, audio/Spotify code or other speculative change was added. The original source/artifacts remain preserved; 0.2 is built from a separately verified source copy. Version/build-path changes have no intended runtime effect.

1. Close VitaSpot manually first. Do not accept a game's force-close prompt.
2. Install 0.2 over the **Service Probe** bubble, not over VitaSpot, and launch it once.
3. Check ux0:data/VitaSpot-ServiceProbe/launcher.log. It should now identify probe=2. Report start_bg_app's new result.
4. If start returns zero and heartbeat.log appears, compare its last sample/PID at two times about 20 seconds apart after the launcher returns. The worker should record its limit stop after roughly two minutes awake.

Do not launch repeatedly, test games, peel the probe's LiveArea bubble or test power/sleep yet. A zero launch result alone is not a worker-lifetime pass. If it remains negative, use that result to diagnose the launch path before changing worker memory or ownership. [Full worker design and bounded-test limits](service-lifetime.md).

## Verification

Native Release compilation passed. Disassembly shows `movs r0, #0` immediately before the sole call to sceBgAppUtilStartBgApp. Three package/native-evidence checks passed: actual zero argument, unchanged worker/unrelated payloads, and source/ELF/version/hash consistency. The existing host bounded-worker contract is reused because its source and packaged worker SELF are unchanged; no new worker runtime pass is claimed.

VPK SHA-256: 27153cd82e1c8be9d6dbafc1feeb067b28f67814b0ad2dc9b3a721b5b6a1d241. Debug artifacts, source diff and disassembly accompany the retained build/package evidence. Device acceptance of zero and advancing worker execution are confirmed by user report; the bounded stop record is now verified from the supplied device log; post-stop shell health is confirmed by user report; process exit is not independently measured.
