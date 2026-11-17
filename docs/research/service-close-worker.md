> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Worker-only LiveArea persistence comparison

Both the unflagged control and the parent-only candidate failed to preserve heartbeat progress after LiveArea closure, with normal shell behavior by user report. This package tests the worker placement separately. It is a hypothesis, not a claimed fix.

The package is reconstructed from the unflagged close-control.1 VPK. Parent ATTRIBUTE=0x00008000 (the unsuccessful parent-only bit is removed); worker ATTRIBUTE=0x04008000 (candidate 0x04000000 added while preserving the original bit). Both APP_VER values advance to 01.05. Both executables, 4 MiB SELF budgets, 256 KiB heaps, assets, title pairing and bounded worker behavior are unchanged. Launcher still logs probe=2. There is no Spotify code, audio ownership, graphics, network setup, plugin or changed power policy.

Three package/evidence tests pass, including exact metadata-only changes, rejection of a parent-side flag and hashes. This reuses the native SDK/source evidence of the proven 0.2 executables. Device result pending. The existing SFO research table motivates this bit; its placement and semantics remain device hypotheses: https://www.psdevwiki.com/vita/System_File_Object_(SFO)_(PSF)#ATTRIBUTE .

## Test once

1. Close VitaSpot manually. Install over the separate Service Probe bubble and launch once.
2. Note the fresh worker PID/sample in ux0:data/VitaSpot-ServiceProbe/heartbeat.log. Close the Service Probe LiveArea page promptly, preferably before sample 10 (20 seconds), using the same normal peel gesture. Do not press Start again.
3. Reopen the log after 20 seconds and note whether samples advance under the same PID. Report the approximate closure sample and the later sample.
4. Let the bounded run finish (normally sample=60, about two minutes from launch). Send launcher.log and the final heartbeat.log and confirm Home folders and an ordinary app still work normally. If progress stops early, send that existing log; do not relaunch just to repeat the failure.

If the run reaches sample=60 before closure, it does not test survival. No game, active foreground UI force-close, power-button or sleep test is requested. Old memory.log is unrelated. A worker retained by this flag still has its own bounded limit; do not infer permanent residency or playback compatibility.

VPK SHA-256: c92b9a05c54fbb1a3c5f088da91e8b02f9b81d53101a8aa1d21c438b9c9adbf4.

## Device logs received, 11 November 2026

The launcher reports load=0/start=0 under PID 1450929. The worker log contains samples 0..9 under PID 1583323, spanning 18.028250 seconds. It ends with a running record and has no final stop line. Raw files and SHA-256 digests are preserved in artifacts/service-probes/device-evidence/probe04-close-worker1/. This confirms launch and early progress, not survival/failure across closure without the user's timing and later-log observation. Clarification and post-test shell health are pending; no repeat launch is requested.

## User confirmation: worker-only persistence passed

The user confirms samples continued all the way to 60 after closing and folders/apps remain normal. This records a LiveArea page-removal survival pass for worker-only ATTRIBUTE=0x04008000 on this device, compared with failed unflagged and parent-only controls. The raw archived log contains samples 0..9 only; later continuation and shell health are user observations. No final stop line or independent process-exit measurement is claimed. This is not proof of force-closing an active graphical frontend, game coexistence or sleep playback. The candidate bit is now carried into the separate backend initialization diagnostic.
