> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

# Isolating package attributes after a working rollback

**Device result:** the user reports closing, other apps/folders and reopening all work on this control. A separate PS/Home check stops music and returning resumes it. This supports the ATTRIBUTE configuration as a contributor to the close regression but does not isolate its two changed bits. The next [graphics-gated Home experiment](home-lifecycle.md) has separate source changes and remains unverified on device.

9 November 2026: the user confirms the 0.0.4 recovery build closes and reopens normally. That establishes a working device baseline for lifecycle comparisons. The earlier relaunch.1 executable is associated with a shell-wide failure after closing; a forced restart restored other app launches and Home-screen bubble folders. The precise cause is still unconfirmed.

## The next controlled comparison

VitaSpot-0.0.5-attribute-control.1.vpk (historical lab reference: `../artifacts/experiments/VitaSpot-0.0.5-attribute-control.1/VitaSpot-0.0.5-attribute-control.1.vpk`) has **exactly the same executable and resources as relaunch.1**, with two SFO field changes:

| Field | Failing relaunch.1 | Attribute control |
| --- | --- | --- |
| ATTRIBUTE | 8 (0x00000008) | 32768 (0x00008000), the working baseline's value |
| APP_VER | 01.03 | 01.05, for installation over recovery.1 |

This removes no-suspend-on-deactivation and restores the original communication-zone setting together. They are two bits within one configuration field; a successful test would not distinguish which bit matters. It does establish whether this package configuration contributes while keeping code unchanged. All other metadata, including parental level, remains identical to relaunch.1.

The executable still contains the newer BGM ownership, sink Drop and audio-thread keep-awake changes. None is added or removed for this comparison. No rebuilt Rust/C code, new thread, plugin, graphics mode or memory budget is introduced. Its in-app version remains 0.0.5-rc.1-relaunch.1 because its executable is byte-identical. Identify this test by its VPK filename and package version 01.05.

This is a close/reopen control, not a background-playback release. Home, power-button playback and game coexistence are not acceptance targets for this package.

## One short device check

Keep recovery.2 (historical lab reference: `../artifacts/recovery/VitaSpot-0.0.4-recovery.2/VitaSpot-0.0.4-recovery.2.vpk`) accessible in VitaShell before testing. It contains the exact executable confirmed working in recovery.1, with only APP_VER raised to 01.06 so it can replace this 01.05 test without a downgrade. Existing recovery.1 remains unchanged and is still suitable if no test has been installed.

1. Install attribute-control.1 over VitaSpot. Preserve ux0:data/VitaSpot/.
2. Open it and play briefly. Close it once through LiveArea.
3. Check one Home-screen folder and open another app. Reopen VitaSpot once only if the shell remains normal.
4. If the shell hangs again, stop the test. After recovering the shell, restore recovery.2. Do not repeat the failing cycle or install the withdrawn bgtest.1/relaunch.1 candidates.

This package may still reproduce the previous hang because its playback executable is unchanged. The user may keep using the confirmed recovery build and defer this experiment. No extra restart is needed while the current shell is healthy.

If accessible after a failure, retain the final startup.log/startup.prev.log stage/PID lines. They come from the unchanged diagnostic executable. Do not relaunch repeatedly to collect more logs and never share credentials.json.

## Interpreting the outcome

- **Close, other apps/folders and reopen work:** the ATTRIBUTE change contributes to the regression under this scenario. It does not prove every BGM shutdown path safe, or permit restoring either lifecycle bit without further review.
- **The shell hangs again:** removing the lifecycle flags is insufficient. Keep them removed and investigate the background ownership/audio shutdown code using the working baseline as the control.
- **The test fails before playback:** record that separately; it is not the same close-after-playback result.

Release and game/sleep experiments remain on hold. Only this narrow comparison is prepared following recovery confirmation.

## Local verification

`py scripts/package_attribute_control.py` verifies immutable input hashes, preserves every executable/resource byte, checks SFO changes exactly, validates CRC/privacy/normal app memory metadata, and creates both the control and higher-version rollback. Existing output directories cannot be overwritten. Verification JSON and SHA256SUMS accompany both VPKs.

Three paired-package tests passed with `py -m unittest discover -s tests -p test_attribute_control.py`: exact control payload, exact original rollback payload, and hashes of packages and packaging sources. No native rebuild was needed. Device observations are reported separately above and were supplied by the user.

- Attribute control SHA-256: 048740bd894813648c2c5548c23b820b298f3914fe4b771082c75646c5a260a8.
- Recovery.2 SHA-256: 456a27592c0442721fb6b430b47a67a9481081a76f1f3d4d07bf02b49ec7af6d.

The [PARAM.SFO reference](https://www.psdevwiki.com/vita/PARAM.SFO) documents the attribute bit names with firmware/speculation caveats. Inspection of the actual two VPKs establishes the values and byte-identical executable comparison.
