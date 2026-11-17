> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Relaunch hang: separate diagnostic

**Withdrawn from further device testing, 9 November:** the user reports that the first launch works, but closing this diagnostic is followed by hangs when opening other apps and folders appearing empty. Use the [recovery guidance](recovery-and-shell-hang.md). Historical build/test instructions below are retained for investigation, not a request to repeat the failing cycle.

9 November 2026: the user reports that closing the earlier build and reopening it leaves the Vita's own loading screen running before the app appears. The user has not installed bgtest.1. The exact older package is not independently confirmed; RC1 is the current earlier candidate. This report must not be attributed to the untested gdc/background experiment.

## Original diagnostic procedure (superseded by recovery)

VitaSpot-0.0.5-rc.1-relaunch.1.vpk (historical lab reference: `../artifacts/experiments/VitaSpot-0.0.5-rc.1-relaunch.1/VitaSpot-0.0.5-rc.1-relaunch.1.vpk`) is an isolated relaunch diagnostic. Defer bgtest.1 until this startup problem is understood. The diagnostic retains the normal game application class, normal executable budget, default heap, real streaming backend and existing account folder. Its package version is 01.03 and in-app version is 0.0.5-rc.1-relaunch.1.

It removes only the experimental disable-system-suspend ATTRIBUTE bit from RC1, retaining the no-suspend-on-deactivation bit: ATTRIBUTE becomes 8 rather than 524296. BGM ownership and the playing audio-thread's inactivity tick remain. This tests a lifecycle hypothesis; it is not an established root cause. It does not add game coexistence or promise power-button playback.

1. Keep the old VPK. Preserve ux0:data/VitaSpot/ and optionally back it up privately. Do not delete saved credentials or caches as an initial workaround.
2. Install this diagnostic over VitaSpot with VitaShell.
3. Save/close any game and shut the Vita down normally, then power it on once. This clears any already-stuck process; it is a test baseline, not a required permanent workaround.
4. Open VitaSpot, confirm normal playback, close it using the LiveArea peel/close gesture, then reopen it. Repeat three times if successful. Record whether the first launch works and whether subsequent launches hang.
5. If it hangs again, use VitaShell to inspect ux0:data/VitaSpot/startup.log and startup.prev.log. Note the last stage and PID in each, whether a new PID appeared, and whether the files changed on the failed launch. Paste those stage lines; they contain no account data. The separate vam.log can contain account identifiers and must be redacted if shared.

## Diagnostic meaning

The first instruction sequence in wrapped main creates/rotates startup.log and records a process ID and monotonic time. Each stage opens, writes and closes the log, so a forced close does not depend on buffered messages being flushed later. Only the current and prior launch are retained. Stages cover main entry, platform initialization, SDL initialization, window creation, renderer creation, app/backend initialization, first frame presentation, normal backend shutdown and main return.

- No new log/PID on a failed launch: execution may be blocked before main or writing the log may have failed. It is not proof of a Spotify login failure.
- A begin marker without its corresponding return marker: narrows the stall to that startup operation.
- First-present-ready: at least one frame was submitted; investigate the app's authentication/catalogue state separately if the visible spinner continues.
- Missing shutdown/main-return after a LiveArea close: can be expected when the OS forcibly terminates the process. These markers are useful for a normal in-app exit, not a claim that every OS close runs cleanup.

This trace starts at main, after the loader and C runtime. It cannot diagnose a loader or heap initialization hang directly. The earlier build lacked these early checkpoints: vam.log starts only after graphics/font setup reaches backend initialization. A stale vam.log alone cannot distinguish a failed launch from a network issue.

## Build and validation

Run `py scripts/build_relaunch_probe.py` after the existing native RC1 build. A separate build/relaunch-probe tree uses the pinned backend archive and adds linker wrappers without altering any of the five patched upstream source files. The helper validates source reconstruction, frozen RC1 hash, package CRC/resources, app class/version/attributes, normal SELF budget and required linked wrapper symbols. The frozen RC1 and bgtest.1 artifacts remain unchanged. Debug ELF/VELF and build logs remain local.

Run `py -m unittest discover -s tests -p test_*probe.py` and the existing release-validator tests. Compiler/binary tests do not establish that a Vita can relaunch successfully. Keep the diagnostic experimental until the device cycle passes.

Native build and all 15 release/residency/relaunch binary validation tests passed. Disassembly confirms startup calls through the linked wrappers. VPK SHA-256: 4916ee524a275e9db3244ba68ff4f06abd7c618a58205793834a8ccdd41e48ca. The only native compiler/linker warning was the existing SDK crtn.o GNU-stack note. Device behavior is unverified.

The suspend flag names come from the community [PARAM.SFO research](https://www.psdevwiki.com/vita/PARAM.SFO), which explicitly notes uncertainty and firmware differences. Removing the flag is a controlled comparison, not proof it caused the hang.
