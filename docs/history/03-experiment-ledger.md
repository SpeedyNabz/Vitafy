> Archived lab handover. Read [current status](../status.md) first; no historical VPKs or raw device logs are distributed here.

> Current context, 17 November 2026: see [current findings](../research/current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Experiment ledger

All user-performed Vita/device observations below are attributed to the same **real, modded PS Vita PCH-1003**, confirmed by the user on 11 November 2026. No emulator result is being substituted. Firmware and plugin details are still unknown; Windows mock/native build checks remain host results.

Chronology spans 8-11 November 2026. Some device responses lack a reliable individual
test date. Results below preserve the conversation order without assigning invented
timestamps. Hashes and install metadata are indexed in the generated catalogue.

## Original app, regressions and recovery

| Build/scenario | Change or purpose | Observed result |
| --- | --- | --- |
| Original 0.0.4 development VPK | Real native frontend/backend; title VTAM00001, SFO ATTRIBUTE 0x8000 | User reports the application works perfectly; later background/relaunch concerns narrowed through tests |
| 0.0.5-rc.1 | ATTRIBUTE 0x80008, BGM ownership/cleanup, keep-awake and release-candidate changes | Launching a game closes VitaSpot and stops playback; not a background/game release |
| rc.1-bgtest.1 | Experimental gdc/74 MiB SELF, 32 MiB heap and memory logging | Never device-tested; withdrawn; do not infer success from build evidence |
| rc.1-relaunch.1 | Early startup logging; ATTRIBUTE 0x8; normal game memory/class | First launch works, then other apps hang and Home bubble folders appear empty after close; forced restart restores normal behavior |
| Recovery.1 | Original executable/resources, install version 01.04 | User confirms normal close/reopen restored |
| attribute-control.1 | Exact relaunch.1 executable, ATTRIBUTE restored to 0x8000, APP_VER 01.05 | Close, folders, other apps and reopen work. At Home music stops and resumes on return |
| Recovery.2 | Original executable/resources, APP_VER 01.06 | Retained rollback, not a separately claimed new hardware pass |
| home-control.1 | New graphics-gated executable with base ATTRIBUTE, APP_VER 01.07 | Binary comparison control; not the recommended rollback |
| home.1 | Graphics lifecycle gating; ATTRIBUTE 0x8008; APP_VER 01.08 | Home audio continues; returning works; manually closing directly from Home while playing and reopening work |
| home.1 game-triggered force-close | Game asks to close the playing app | Shell hangs; forced reset required. Do not repeat routinely |
| Recovery.3 | Original executable/resources, APP_VER 01.09 | Retained rollback |
| home.2 | Same Home gate but AppMgr BGM acquire/release calls blocked; APP_VER 01.10 | Home music stops; returning and normal closing work. Feature regression; rejected |
| Recovery.4 | Original executable/resources, APP_VER 01.11 | Retained rollback |
| home.1-restored | Exact home.1 executable/resources, only APP_VER raised to 01.12 | Prepared to restore Home audio over home.2; no new device-install confirmation. Retains known game-close failure |
| Recovery.5 | Original executable/resources, APP_VER 01.13 | Latest foreground-only rollback package; no separately claimed new device test |

The user's successful home.1 close did **not** require returning to the app or
pausing first: they went Home while playing, closed there, and reopened normally.
The failing scenario is specifically game-triggered forced closure while playing.
Empty folders were Home-screen bubble folders, not VitaShell filesystem contents;
no data-loss conclusion is supported. An earlier question answer referred to the
older build before the new diagnostic was tested; do not attribute it to bgtest.1.

## Separate service preflight

Parent title VTSV01001 is gdc; worker VTSV01002 is gdd. Neither replaces the main
VTAM00001 title/account directory. Early probe code is C, without audio/network setup.

| Probe | Purpose/change | Device evidence and limits |
| --- | --- | --- |
| 0.1 | Load BG_APP_UTIL; StartBgApp(1), following installed header comment | Launcher PID 23406355: load=0, start=0x80106501. No heartbeat. Rejected request, not an established worker I/O/memory failure |
| 0.2 | StartBgApp(0); exact unchanged worker/resources, parent APP_VER 01.01 | Launcher PID 2436767: load=0/start=0. Raw heartbeat PID 9055701 contains samples 0..60 and `status=stopped reason=limit` after 120.313727 s. User confirms normal folders/apps afterward |
| Backend static audit | Link all 25 real C ABI entry points without frontend/graphics | 15,893,348 bytes in ELF load segments (15.16 MiB), before runtime allocations. Host native link audit only; not a VPK/runtime result |
| 0.3-memory.1 | Real backend linked inactive; request 24 MiB SELF, 4 MiB heap; two page-touched 1 MiB blocks | Launcher PID 12398267 start=0; worker PID 6826821 allocates both, records release, ten observations, and bounded stop with allocation_result=0. User confirms shell normal. Snapshots report user pool 2 MiB, not remaining malloc capacity |
| 0.4-close-control.1 | Exact 0.2 executables; APP_VER 01.03, no persistence bit | Launcher PID 11020359 starts worker PID 14428297. Raw log ends at sample 3. User confirms it remained there after closing LiveArea and at least 20 s; folders/apps normal. Survival fails |
| 0.4-close-parent.1 | Only parent gets candidate 0x04000000; parent ATTRIBUTE 0x04008000, worker 0x8000; APP_VER 01.04 | Observed launcher PID 1453447, worker PID 1519285, samples 0..52, 104.536359 s. User confirms no increase after closing, folders normal. Parent-only survival fails. Raw files disappeared before copy; extracted results only |
| 0.4-close-worker.1 | Parent restored to 0x8000; only worker ATTRIBUTE becomes 0x04008000; APP_VER 01.05 | Raw launcher PID 1450929 start=0; worker PID 1583323 raw samples 0..9. User subsequently confirms continuation to 60 after closing and normal folders/apps. Survival passes by user report; no archived complete 0..60 log or independent final stop proof |
| 0.5-runtime.1 | Real init/discovery/shutdown, audio acquisition blocked; 48 MiB SELF, 24 MiB heap; APP_VER 01.06 | Launcher PID 4861283 start=0; worker PID 668689 init=0, auth starting->logged_out, 15 observations, after_shutdown, final init_result=0/log_failed=0/guard_wait=0. Filtered backend log confirms discovery and joined runtime. User confirms normal shell |
| 0.6-playback.1 | Real audio enabled; same startup-tested memory/worker flag; isolated pairing, four-minute trial; APP_VER 01.07 | Cross-build and three native/package tests pass. No device playback response received yet |

The memory.log supplied during the close-control test identified a separate probe=3
run under PID 9121239; it does not prove anything about closure. Likewise, the old
heartbeat PID 9055701 supplied with the memory test belonged to 0.2. Correlate probe,
PID, log content and the installed package; filenames alone are insufficient.

## Runtime 0.5 quantitative evidence

| Measurement | Observed value |
| --- | --- |
| Maximum sampled allocator used bytes | 1,084,504 (~1.03 MiB), idle/unpaired |
| Grown allocator arena | 1,155,072 bytes |
| Idle free chunks in that arena | 70,568 bytes |
| Allocator used after shutdown | 8,040 bytes |
| Before/after shutdown timestamp difference | 0.8167 seconds |
| User-pool snapshot before init / idle / after shutdown | 5 MiB / 2 MiB / 5 MiB |

These are sampled allocator/pool statistics, not total process residency, an exact
high-water mark, a maximum permitted gdd budget or proof of memory sufficiency for
TLS/session/decoder/audio playback. A small free-chunk number in the grown arena
does not mean the configured 24 MiB heap is almost full.

## Host/native checks actually performed

- Desktop mock contract: three scenarios, 1,525 assertions; Premium, delayed login
  and free-account rejection. These do not contact Spotify or output real audio.
- Desktop lifecycle smoke: login, Home, Now Playing and successful shutdown after
  repairing the reproduced allocator mismatch.
- Home graphics-gate host tests: unchanged framebuffer while backgrounded, resumed
  updates after foreground return, and no extra present after foreground/background
  quit. This is not an atomic GPU/focus guarantee on hardware.
- Native frontend/backend cross-build, VPK CRC/resource/privacy/metadata checks and
  pinned source reconstruction. Six release-package tests were recorded earlier.
- Bounded worker host contract: limit, requested stop, immediate stop, write failure
  and invalid input. Memory host contract includes allocation/report failures and
  cleanup on all tested paths.
- Each service version has its own package/native-evidence checks; versions 0.2,
  memory, close controls/comparisons, runtime and playback each recorded three tests.
  Runtime audio denial is checked in actual assembly/symbols; playback verifies
  real imports and a monitor with no forced ExitProcess call.

The superseded strong-import 0.1 package is preserved as build history; it is not the
weak-import package used in the user's launch test. No emulator observation is
substituted for a user device report.
