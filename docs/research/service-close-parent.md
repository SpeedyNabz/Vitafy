> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Parent-only LiveArea persistence comparison

The user confirms that close-control.1 stopped advancing at sample=3 after normal LiveArea page removal, with folders/apps remaining normal. No game was used.

The SFO research table describes candidate bit 0x04000000 as keeping a background app alive when LiveArea is removed: https://www.psdevwiki.com/vita/System_File_Object_(SFO)_(PSF)#ATTRIBUTE . The table includes speculative entries. Parent placement and behavior on this firmware are a hypothesis, not established SDK guarantees. Installed VitaSDK headers did not provide a named constant for this bit. The inspected GrapheneCt sample's parent ATTRIBUTE=16809984 does not include it, so that sample does not prove its use.

VitaSpot-ServiceProbe-0.4-close-parent.1 changes the parent ATTRIBUTE from 0x00008000 to 0x04008000, preserving the existing bit. Worker ATTRIBUTE stays 0x00008000. Both APP_VER values advance to 01.04. Both executables, budgets/heaps, title pairing, assets and worker behavior are byte-identical to close-control.1. No no-forced-kill or disable-system-suspend flag is introduced. There is no audio, real backend or active graphical frontend. The launcher still logs probe=2 because it is unchanged.

Three package checks passed: exact expected metadata-only delta, rejection of a worker-side flag change, and evidence hashes. No native rebuild was necessary. Device result pending.

## Device check

1. Manually close VitaSpot. Install over the separate Service Probe bubble and launch once.
2. Promptly read ux0:data/VitaSpot-ServiceProbe/heartbeat.log and note the fresh worker PID/sample while sample is below 30. Do not relaunch.
3. Close the Service Probe LiveArea page with the same normal peel gesture used in the control test. Note the approximate sample at closure.
4. Reopen heartbeat.log after 15-20 seconds. Check whether samples increase under the same PID after closure. Send launcher.log and heartbeat.log with that observation.
5. Allow the bounded run to end (60 two-second waits, roughly two minutes from launch), and check Home folders and an ordinary app. Report any regression rather than repeating it. Do not launch a game or test power/sleep.

The final stop record alone does not prove survival after closure: before/after samples are required. If the limit was reached before closure, the result is inconclusive. Old memory.log is unrelated.

VPK SHA-256: feb4a909e2793ef0e947052846358acbac9db2e41f8d2cb630d1c68253d2d584.

## Device logs observed, 11 November 2026

Launcher PID 1453447 reported module load=0 and start=0. The heartbeat contained all samples 0..52 under PID 1519285, spanning 104.536359 seconds with increasing timestamps. No final stop record was present. This establishes startup/progress; closure timing, post-close advance and shell health require user observation. The original Downloads files became inaccessible before archival, so observed-result.json retains the extracted findings only; no raw-file copy or hash is claimed. Evidence: artifacts/service-probes/device-evidence/probe04-close-parent1/. No repeat launch is requested.

## User clarification: parent-only survival test failed

The user confirms folders were fine and samples did not keep increasing once the app was closed. Combined with the supplied partial log (last sample=52), this records a failed parent-only survival observation with shell health preserved. The exact termination reason is not independently measured. The next controlled comparison restores the parent's original ATTRIBUTE and puts the candidate bit only on the worker, retaining both executables. No game or audio test follows from this result.
