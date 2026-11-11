> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

# Service Probe 0.4-close-control.1

The 0.3 memory probe's intended stop and normal post-stop shell behavior are confirmed. This next check isolates removal of the launcher's LiveArea page. It is not a test of killing an active graphical frontend, and does not establish game-triggered closure safety.

The package reuses the exact device-proven 0.2 launcher and heartbeat worker; both SFO install versions alone advance to 01.03 to install over 0.3. The original 4 MiB SELF budgets, 256 KiB heaps, attributes, title IDs and assets are unchanged. The launcher still logs probe=2; that is expected. No real backend, audio, network initialization, graphics or persistence flags are added. It returns to Home immediately and the worker runs for at most 60 two-second waits. Existing memory.log is historical and irrelevant to this check.

1. Manually close VitaSpot and install this VPK over the separate Service Probe bubble.
2. Launch once. It returns to Home. Promptly read ux0:data/VitaSpot-ServiceProbe/heartbeat.log and note the worker PID and current sample; confirm a fresh running record before sample=30. This log is overwritten on each launch. Do not relaunch during this run.
3. Return to the Service Probe LiveArea page without pressing Start again. If its normal peel/close handle is available, close that page once, while the worker is still within its two-minute run. Do not use a game's close prompt. If there is no close handle because the launcher has already returned, report that; do not substitute another action.
4. Reopen heartbeat.log after about 15-20 seconds. Report whether the sample advanced under the same PID after closing the page. Note roughly which sample you closed at. If the normal sample=60 limit was already reached before closure, the result is inconclusive.
5. Let the remainder of the two-minute run finish, then send launcher.log and heartbeat.log and confirm Home folders and an ordinary app work normally. If the worker stops early, that is useful control evidence; do not repeat it or force a game launch.

The worker has no audio ownership and no game is involved. Stop if shell behavior changes. A final normal limit record alone does not prove continuation after page removal; the before/after sample observations are needed.

Verification: three package/evidence tests passed, including rejection of a substituted worker and byte-for-byte checks of every executable/resource. No recompilation was needed. The original SDK snapshot and execution evidence remain those of probe 0.2. The new package has no device closure result yet.

SHA-256: c7b16209e402c1b29f22af441f020cceefd5bac71285023a339267a5609fc703.

## Supplied device log and user clarification: no survival across page removal

The user supplied launcher.log with probe=2, PID 11020359, module load=0 and start=0. heartbeat.log contains only samples 0..3 under worker PID 14428297, ending at time_us=6070755 with no stop record. This establishes startup and roughly six seconds of execution. It does not establish page removal timing, whether the log was copied before completion, or whether the worker stopped at closure. Do not mark closure survival passed or failed without those observations. The accompanying memory.log identifies a separate probe=3 run under PID 9121239 and is not closure evidence. Original files and hashes are retained in artifacts/service-probes/device-evidence/probe04-close-control1/. The user subsequently answered yes to the bundled clarification: they closed the LiveArea page, reopened the log at least 20 seconds afterward with it still ending at sample=3, and Home folders/another app remained normal. This establishes a failed survival control by observed behavior, without independently identifying the process termination reason. The next comparison adds only candidate bit 0x04000000 to the parent SFO and advances install versions. See service-close-parent.md.
