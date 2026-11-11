> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

# Game-launch residency experiment

**On hold:** a separate relaunch diagnostic was reported to leave the shell unable to open other apps normally, with apparently empty folders. bgtest.1 remains untested and should not be installed for further tests. Use the [recovery guidance](recovery-and-shell-hang.md) first.

Priority update: bgtest.1 has not been tested. The user reports a separate earlier-build relaunch hang at the Vita loading screen. Test the [relaunch diagnostic](relaunch-probe.md) first to avoid conflating the two issues.

9 November 2026: the user reports that launching a game closes VitaSpot and stops music in 0.0.5-rc.1. This is a failed cross-game test. Home, other-app and power-button results are still unspecified.

## Test package

VitaSpot-0.0.5-rc.1-bgtest.1.vpk (historical lab reference: `../artifacts/experiments/VitaSpot-0.0.5-rc.1-bgtest.1/VitaSpot-0.0.5-rc.1-bgtest.1.vpk`) is a separate private experiment. It uses the same title ID and account directory. It updates the package to 01.02; Settings/logs identify the experiment as 0.0.5-rc.1-bgtest.1. The RC1 build, source patches and frozen release bundle are unchanged.

1. Keep the RC1 VPK and working 0.0.4 VPK. Back up ux0:data/VitaSpot/ privately if desired.
2. Install the experimental VPK over VitaSpot using VitaShell. Do not uninstall/delete its account folder.
3. Confirm it opens, renders correctly, and plays at least two tracks. Browse several pages to exercise the smaller heap.
4. Press PS, launch the same game that previously closed VitaSpot, and listen through a track change. Record any close-app prompt, whether the game starts, and whether VitaSpot remains open.
5. Return to VitaSpot and check rendering and controls. If playback stopped, report whether VitaSpot closed or remained open.
6. Test power-button behavior separately. It still cannot stream through true hardware suspend.

It creates ux0:data/VitaSpot/background-budget.log five seconds after startup. This bounded diagnostic records the app mode and free/total user, physical and CDRAM budgets every 30 seconds for one hour, overwriting the prior diagnostic on launch. These values are partition headroom; free user memory excludes the reserved malloc heap and does not measure unused space inside that heap. Retain it immediately after a failed test. The separate vam.log may contain identifiers; redact it before sharing.

If the experiment fails to open or becomes unstable, restore the previous VPK. VitaShell may warn about its lower package version; preserve private data before any reinstall operation. No firmware/plugin change is part of this experiment.

## Why these changes

RC1's CATEGORY=gd and zero SELF memory budget identify a normal game-budget executable. BGM port ownership alone does not change its launch class. The experimental wrapper changes CATEGORY to gdc and sets a nonzero SELF Control Info type 6 memory budget to 0x12800 KiB (74 MiB), the upper system-app budget documented by the installed vita-make-fself. Its Control Info attributes remain zero. Merely editing EBOOT_APP_MEMSIZE in PARAM.SFO would not set these executable boot parameters.

Newlib's linked heap override was previously undefined. Its startup defaults to a 128 MiB allocation, so the isolated wrapper defines _newlib_heap_size_user as 32 MiB. The current native ELF contains approximately 18.1 MiB of text/data/BSS; the rest of the 74 MiB must also accommodate threads, modules and other allocations. This is a capacity experiment, not a measured safe memory ceiling. A smaller image cache, lifecycle-aware graphics or a separate player process may still be needed.

The existing SDL renderer has not been established to handle system-app framebuffer ownership. Check startup and return-from-LiveArea rendering explicitly. Large-memory games can also displace smaller apps. No claim of universal game compatibility follows from a successful first test.

## Reproduction and evidence

Run `py scripts/build_resident_probe.py` after the native RC1 build. It verifies the pinned source/patch reconstruction and frozen RC1 hash, uses a separate build/resident-probe-native tree, links the real backend, verifies SELF budget and SFO metadata, compares every bundled resource against RC1, and writes verification.json and SHA256SUMS.txt. It refuses to overwrite an existing experiment directory. Debug ELF/VELF files remain in the separate build tree. Run `py -m unittest discover -s tests -p test_resident_probe.py` for binary validation.

Native build and all 11 release/residency binary validation tests passed on this host. The VPK SHA-256 is e4a535ccbf65ff9d5a446c51a54230beb49909fa64a2a3201c687deee92bce2b. The original RC1 hash remains unchanged. Device startup, playback, memory stability and game coexistence remain pending.

## Primary references

- [GrapheneCt BG-App-PSV, pinned CMake](https://github.com/GrapheneCt/BG-App-PSV/blob/49dbde8ae9ac8fc2adbd5884a0fa9313046bac84/CMakeLists.txt) and [README](https://github.com/GrapheneCt/BG-App-PSV/blob/49dbde8ae9ac8fc2adbd5884a0fa9313046bac84/README.md): gdc main application and separate gdd background application, executable boot budgets, stated game coexistence. This experiment uses only a gdc main app; it does not implement that sample's service or copy its source.
- [VitaSDK toolchain SELF generator](https://github.com/vitasdk/vita-toolchain/blob/008ddc2d7fd4bd16631d1e49cacf308de6f545f1/src/vita-make-fself/vita-make-fself.c) and [packed SELF structures](https://github.com/vitasdk/vita-toolchain/blob/008ddc2d7fd4bd16631d1e49cacf308de6f545f1/src/self.h): memory budget arguments and Control Info type 6 layout. Installed executable help and vita.cmake confirm support in this SDK.
- [VitaSDK newlib heap implementation](https://github.com/vitasdk/newlib/blob/2e428297c0b6aefd830c5a75a7daa7e774562a42/newlib/libc/sys/vita/sbrk.c): default heap and override. The baseline ELF has an unresolved weak override; the experimental ELF must define it.
- [libSceMusic](https://github.com/GrapheneCt/libSceMusic): current successor to libShellAudio, using SceShell's file player. That is a fallback architecture reference, not an implemented Spotify streaming service.

The package configuration is an engineering hypothesis based on these sources. Only a physical-device test can establish startup, rendering, streaming and coexistence on this console.
