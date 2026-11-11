> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

# Executed validation

Hardware clarification, 11 November 2026: the user confirms all reported Vita/device tests were on a **real modded PCH-1003**, not an emulator. Firmware and installed plugins remain unspecified. Windows mock/native build/package checks are host-side evidence. The historical 8 November setup/mock sections below describe local checks without a connected Vita/account; subsequent device sections contain the user's physical-hardware observations.

## Worker persistence and backend runtime diagnostic

Worker-only close persistence passed by user observation through sample 60 with normal shell behavior; the archived log contains 0..9 only. Backend runtime diagnostic 0.5-runtime.1 cross-build passed with the exact home.1 real archive. Three native/package tests pass, including actual vam_init/vam_shutdown/event release calls, actual heap override and SELF budget, audio-denial wrappers/absence of original acquisition symbols, frozen payload/source evidence and pairing. A test initially expected only one ARM encoding of -1; it was corrected to accept the observed equivalent mov.w r0,#4294967295 encoding, then passed. No VPK code change was required. Supplied runtime logs now pass real startup/logged-out discovery/shutdown; the user confirms normal shell behavior. Maximum sampled allocator use is 1,084,504 bytes idle, not a playback peak. Bounded audio-enabled service 0.6-playback.1 cross-build and three native/package checks pass; device pairing/audio/cleanup remain pending. See service-runtime.md.

## Service memory diagnostic 0.3-memory.1

Native cross-build passed against SDK snapshot sdk-snapshot-20260825.611.1 and the exact home.1 Rust archive. Host tests cover allocation failures and report failures with cleanup. Three independent artifact tests passed: actual SELF/SFO payloads and pairing, actual zero launch argument/backend retention/4 MiB heap, and source/asset consistency. Supplied device logs now confirm accepted launch, both 1 MiB allocations, successful memory snapshots and the intended bounded stop with allocation_result=0. This verifies boot with the candidate metadata, not an exact physical grant or remaining malloc capacity. The user confirms normal post-stop shell behavior; no runtime backend initialization or player compatibility is claimed. See service-memory.md.

## Setup and compilation

- Downloaded source from the canonical VitaSpot repository; exact commit is in source-lock.json.
- Downloaded the three pinned official SDL development archives and checked recorded SHA-256 values. SDL2 and SDL2_image matched release asset digests; SDL2_ttf's recorded hash came from the HTTPS release download because the API supplied no independent digest.
- Initialize-Desktop.ps1 -Offline verified cached archives and prepared dependencies.
- MSVC Release build produced VitaSpotPreview.exe and vita_spotify_contract.exe.

## Contract scenarios

| Scenario | Checks | Failures | Skipped |
| --- | --- | --- | --- |
| Simulated Premium account | 560 | 0 | 0 |
| Simulated login transition | 560 | 0 | 0 |
| Simulated free account | 405 | 0 | 0 |

CTest: 3 of 3 scenarios passed. These are upstream contract checks against the mock implementation, not Spotify service or audio checks. Evidence is in build/desktop/Testing/Temporary/LastTest.log.

## UI lifecycle regression

The initial scripted preview rendered frames but exited with 0xC0000374. The SDL string allocator patch corrected the inspected mismatch. After rebuilding, Smoke-Preview.ps1 rendered the login, home, and Now Playing frames with a software renderer/dummy video driver, completed shutdown, and returned success. The home frame was visually inspected at 960x544 and showed the simulated library.

Artifacts: artifacts/mock-preview/login.bmp, home.bmp, now-playing.bmp, smoke.txt, stdout.log, and stderr.log. BMPs are ignored generated evidence. Now Playing can show an empty player because the smoke script does not start a track.

## Native build boundary

Build-Vita.ps1 was run and stopped at its explicit missing-VitaSDK preflight. The pinned nightly Rust toolchain is also not installed. No real backend, ARM executable, VPK, Spotify authentication, audio stream, or device lifecycle was tested. A harmless CMake deprecation warning originates in SDL2_ttf's supplied package configuration; it did not block the Windows build.

## Native verification, 9 November 2026

The earlier missing-toolchain boundary has been resolved. Configured VitaSDK 2026.08 (sequence 2026102473, core 2026.08.1-1), required target libraries, and nightly-2026-10-18 with rust-src. Rust's user-wide default remains stable; the project has its own nightly selection.

- ARM C compiler smoke: ELF and Sony SELF produced successfully.
- Real Rust/librespot backend cross-compiled for armv7-sony-vita-newlibeabihf with build-std.
- Native frontend and compatibility shim linked successfully using the external backend.
- CMake cache confirms Release, Ninja, and VAM_BACKEND=external.
- VPK archive CRC check passed; eboot.bin and required LiveArea/SFO resources are present.
- Packaged eboot.bin exactly matches the build output; ELF is 32-bit little-endian ARM and SELF has the expected SCE header.
- No credentials or logs were included in the package.

Artifact: VitaSpot-0.0.4-dev.vpk (historical lab reference: `../artifacts/native/VitaSpot-0.0.4-dev.vpk`), 17,117,647 bytes, SHA-256 0b533e2ff2c4768c8910bf271062e47476cff02bc6dc3695125769a193409fbe. Verification JSON (historical lab reference: `../artifacts/native/build-verification.json`) and artifacts/toolchain/native-build.log retain evidence.

The first Release build exposed non-fatal warnings in vendored Rust code, a GCC font-cache range warning, and a missing GNU-stack note in SDK startup code. These were retained in the log; this task did not establish whether the font warning corresponds to a runtime defect. Hardware audio, authentication, memory behavior, and suspend/resume remain untested.

Windows compatibility fixes were also verified: the inspected libpng symlinks have hash-matching regular aliases; the build helper accepts zero-exit native stderr warnings and rejects nonzero exit codes under PowerShell 5.1. No device or account was accessed.

## User report and 0.0.5-rc.1

Service Probe 0.1 device result: launcher returns, no heartbeat.log, load_bg_app_util=0 and start_bg_app=0x80106501. This is a rejected launch request, not a worker-lifetime pass. The [0.2 correction](service-launch-result.md) changes the argument from 1 to 0 based on the native reference and observed error, preserving the exact worker/resources. Native compilation and three native/package checks pass; the user now confirms load=0, start=0 and an advancing heartbeat after the launcher returns. The supplied heartbeat.log verifies all samples 0..60 under PID 9055701, increasing timestamps and status=stopped reason=limit after 120.313727 seconds. The user subsequently confirms normal post-stop shell behavior. This verifies the stop path and user-observed shell health, not independent process exit. A native link-only audit retaining all 25 real backend entry points without graphics passed; load segments total 15,893,348 bytes before runtime memory. See service-memory.md. No new device build/playback claim is made.

Latest clarification: on home.1 the user played music, exited to Home, closed VitaSpot directly there and reopened successfully. The failing path is a game's forced close while playing. home.2 stops Home audio but retains normal return/close behavior, so removing the BGM lease is a functional regression. The Home-working executable is repackaged at install version 01.12 with exact payload identity. A separate no-audio [service lifetime preflight](service-lifetime.md) now builds as a gdc launcher/gdd worker under unique titles; its bounded host contract and three native-package checks pass. Native worker launch/lifetime, any Spotify service, and game coexistence remain unverified.

Latest device report for home.1: music continues at Home, return works and ordinary closing works. Game-triggered forced closure while playback is running still hangs the shell and requires a forced reset. That path is failed and must not be reported as fixed. [home.2](forced-close-investigation.md) now disables the actual AppMgr BGM ownership calls in a separately linked control, retaining the identical Home frontend/backend inputs and attributes. Native compilation and four binary/package checks pass; the new policy's device Home behavior and the forced-close cause remain unconfirmed. No repeat game-triggered forced-close test is requested.

Latest device evidence: attribute-control.1 closes, leaves other apps/Home folders functional, and reopens normally. At PS/Home music stops and returning resumes it. The same-executable metadata comparison therefore supports a configuration contribution to the shell regression, with the individual bit/interaction unresolved. The isolated [graphics-gated Home experiment](home-lifecycle.md) now passes native compilation, four desktop integration assertions and five package tests. Its device behavior remains pending; no game or sleep success is claimed.

Latest confirmation: closing and reopening work normally on recovery.1. This establishes a working baseline by user report. The [attribute control](attribute-control.md) preserves relaunch.1's exact executable/resources and changes only SFO ATTRIBUTE (8 to the baseline's 32768) plus APP_VER for installation. A same-executable recovery.2 at version 01.06 is prepared first. All three paired-package checks pass; device results for the control remain pending. No additional native build or pass of the failed diagnostic is claimed.

Recovery clarification: the empty folders were Home-screen bubble folders, and a forced restart restored everything to normal. This supports a temporary shell-state problem; it does not identify the responsible API. No new experiment is requested. Baseline recovery installation and its close/reopen behavior are pending.

Latest report: relaunch.1 opens normally the first time, but after closing it, attempts to launch other apps hang and folders appear empty. This is a shell-wide symptom, not an established Spotify login failure. Device experiments and release are on hold. [Recovery packaging](recovery-and-shell-hang.md) restores the preserved 0.0.4 executable/resources byte-for-byte, changing only SFO APP_VER to 01.04 for installation over 01.03. The package passes CRC, metadata, private-data and complete payload comparison checks. Recovery result, folder type and recovery after normal power-off are pending. No storage deletion/database rebuild/plugin change was performed.

Relaunch follow-up: the user has not tested bgtest.1 and reports an earlier build hanging on the Vita's own loading screen after close/reopen, before the app appears. This is not established to be a Spotify authentication issue. [relaunch.1](relaunch-probe.md) uses the normal game class/budget, removes ATTRIBUTE bit 19 and adds early startup traces. Its native build and all 15 release/residency/relaunch binary tests passed; disassembly confirms calls through the startup wrappers. Device relaunch results remain pending. The frozen RC1 and bgtest.1 hashes remain unchanged.

Follow-up, 9 November 2026: the user reports that launching a game closes VitaSpot and stops playback. Cross-game playback therefore failed in RC1. Other transitions remain unspecified. The isolated [bgtest.1 residency experiment](resident-probe.md) changes app category and executable memory budget, bounds the newlib heap and adds memory diagnostics; its device results are pending.

The user subsequently reported that the application works perfectly and requested background playback and release cleanup. That confirms baseline core operation by user report; it is not an agent-observed pass of each item in the new background matrix.

The candidate builds successfully with the real backend and Release/Ninja. New SFO values are APP_VER=01.01, TITLE_ID=VTAM00001, and ATTRIBUTE=524296 (0x80008). The font lookup now has an explicit array bound, removing the prior GCC font-cache warning. The SDK startup GNU-stack note and pre-existing vendored Rust warnings remain non-fatal.

Executed after the changes:

- Three mock contract scenarios passed, with the same 560/560/405 check counts.
- Scripted login/home/Now Playing rendering and shutdown passed.
- Six release-validator tests passed, including corrupted SFO bounds, mismatched package version/SELF, private-file/path rejection, and exact source reconstruction.
- Five modified source files reconstruct byte-for-byte from the recorded baseline and three checksum-pinned patches.
- Real VPK CRC, required resources/font notices, matching SELF, ARM ELF, Release configuration, and package version/attributes passed.
- A private RC bundle was assembled with 244 Cargo package license records and available font/SDK/dependency notices. The original working 0.0.4 artifact was preserved.

Candidate VPK SHA-256: 2a9236e16db23a1a0197b4d1383205600dfcd080364e6b9614844d968e438324. New background transitions, power-button behavior, cross-game residency, and public frontend-license clarification remain pending. See device acceptance (historical lab reference: `../release/HARDWARE_TESTS.md`).
