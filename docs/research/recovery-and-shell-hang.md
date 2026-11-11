> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

# Shell hang after closing VitaSpot: recovery first

9 November 2026: the user reports that the relaunch diagnostic opens normally once, but after closing it, other apps also hang at launch and Home-screen bubble folders appear empty. A forced restart was required and restored everything to normal. This is broader than a VitaSpot login/relaunch problem and points to temporary shell state. It does not establish file deletion, filesystem corruption, a specific deadlock or a particular faulty API. bgtest.1 has not been tested.

## Current decision

Update: the user confirms closing and reopening work normally on the recovery build. A single [attribute-field comparison](attribute-control.md), with identical code to the failed diagnostic and a prepared rollback, is now available to isolate the configuration change. Recovery confirmation does not reinstate the withdrawn RC1/relaunch.1/bgtest.1 builds or qualify a release.

Suspend device testing of 0.0.5-rc.1, relaunch.1 and bgtest.1. Do not request another close/reopen stress cycle or add a shell/kernel plugin while system behavior is unstable. Host compiler/package checks passed but did not establish safe device lifecycle behavior. Public release is on hold.

The user has already recovered the shell through a forced restart; no further restart is currently required. Do not format storage, rebuild the database, delete folders or remove account data in response to this report. Do not deliberately reproduce another shell lockup to gather logs.

Once other apps and storage are accessible, the recovery VPK (historical lab reference: `../artifacts/recovery/VitaSpot-0.0.4-recovery.1/VitaSpot-0.0.4-recovery.1.vpk`) restores the exact executable and resources from the 0.0.4 development package previously reported working. Install it over VitaSpot through VitaShell; preserve ux0:data/VitaSpot/ and back that directory up privately if accessible. No uninstall or credential reset is needed for this rollback package.

Only PARAM.SFO's APP_VER changes from 01.00 to 01.04, so VitaShell can install the rollback over the diagnostic's 01.03 without a package downgrade. The running app still identifies itself as 0.0.4. The original ATTRIBUTE=32768, app class, executable boot parameters and all other original metadata are restored. The 0.0.4 baseline remains preserved separately. The user now confirms closing and reopening work normally after recovery.

## What inspection establishes

relaunch.1 already removed the system-suspend-disable flag, but retained no-suspend-on-deactivation, AppMgr BGM ownership, the audio-thread keep-awake tick and the new sink Drop implementation. Its reported failure therefore does not isolate any one of those remaining changes. A LiveArea forced close is not guaranteed to execute Rust destructors or the normal app shutdown sequence.

The sink's normal stop currently joins its audio thread before releasing its audio port and AppMgr BGM ownership. The audio thread calls a blocking sceAudioOutOutput. This makes shutdown ordering worth investigating, but it does not prove that this join caused the shell-wide symptom or that a forced close reached it. Releasing a handle while another thread uses it or forcing thread termination is not an evidence-based fix.

The gdc experiment additionally changes memory/app class and defines a smaller heap; it has no device results. Do not conflate those changes with the failure reported in the normal-class relaunch diagnostic.

## Evidence to retain if accessible

- Confirmed: Home-screen bubble folders appeared empty; a forced restart restored other apps and folder contents. Normal power-off was insufficient/unavailable according to the report that a forced restart was needed.
- Firmware, Vita model and existing storage/plugin setup; do not change that setup during recovery.
- startup.log and startup.prev.log stage/PID lines from ux0:data/VitaSpot/. Missing normal-shutdown stages may be expected after a forced close.
- Whether music was playing or paused before closing, and whether any music continues after closing.

Never share credentials.json. The stage logs contain no account data; vam.log may contain identifiers and requires redaction. Do not keep launching an unstable build just to generate a log.

## Local package evidence

`py scripts/package_recovery.py` verifies the preserved baseline SHA-256, all original resources and metadata, then packages the install-version-only update. It refuses to overwrite an existing recovery folder and checks that every other archive payload, including eboot.bin, remains byte-identical. verification.json and SHA256SUMS.txt accompany the recovery VPK.

Baseline VPK SHA-256: 0b533e2ff2c4768c8910bf271062e47476cff02bc6dc3695125769a193409fbe.

Recovery VPK SHA-256: cc4846ae4097eba29dfb326754601fc6c955c60b6d777b55546d2ca77a8bab1b. Independent reinspection confirmed that all archive payloads match the original except the install-version field. Baseline close/reopen now passes by user report. A same-executable recovery.2 with install version 01.06 accompanies the later attribute control to allow rollback without a downgrade.

[VitaSDK audio API](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/audioout.h) documents blocking output and [AppMgr declarations](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/appmgr.h) document BGM acquire/release. Those interfaces do not establish the cause of this device's shell behavior.
