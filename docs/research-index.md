# Research and historical evidence

Read [current status](status.md) first. The following notes preserve the original research and experiment reasoning. They are not download links or fresh hardware results. Raw device logs, credentials, lab archives and historical VPKs are not published. Machine-local paths are generalized as `<lab>`.

## Current research synthesis and open work

Updated 17 November 2026. [Current findings](research/current-findings.md) combines
the newer app work with the historical service results without upgrading their
evidence. [The open work plan](open-work-plan.md) is the living dependency list
for app reliability and playback alongside supported apps/games.

- [Bluetooth volume and long Next buffering](bluetooth-volume-and-skip-buffering.md):
  reported defect, source hypotheses, candidate fixes, tests and device acceptance.
- [Loading/cache and sorted-playback investigation](loading-cache-and-sorted-playback-investigation.md):
  original baseline; implemented bounds are in [preview.7](releases/preview7.md).
- [Release 0.1.0](releases/0.1.0.md) and [0.1.1-preview.1 test build](releases/0.1.1-preview1.md).
- [Host candidate evidence](evidence/bluetooth-skip-host.json),
  [native candidate evidence](evidence/bluetooth-skip-home-native.json) and
  [reported device defect](evidence/bluetooth-skip-user-report.json).

## Current guides

- [Build and test](building.md)
- [Installation and acceptance](installing.md)
- [Architecture and notifications](architecture.md)
- [Roadmap](roadmap.md) and [open work plan](open-work-plan.md)

## Historical technical research

- [Architecture and selected starting point](research/architecture.md)
- [Isolating package attributes after a working rollback](research/attribute-control.md)
- [Background playback implementation and limits](research/background-playback.md)
- [Home works; game-triggered forced close still hangs](research/forced-close-investigation.md)
- [Home playback with graphics gated](research/home-lifecycle.md)
- [Milestones](research/milestones.md)
- [Native Vita build and deployment](research/native-build.md)
- [Shell hang after closing VitaSpot: recovery first](research/recovery-and-shell-hang.md)
- [Relaunch hang: separate diagnostic](research/relaunch-probe.md)
- [Game-launch residency experiment](research/resident-probe.md)
- [Service Probe 0.4-close-control.1](research/service-close-control.md)
- [Parent-only LiveArea persistence comparison](research/service-close-parent.md)
- [Worker-only LiveArea persistence comparison](research/service-close-worker.md)
- [Service Probe 0.1 launch rejection and 0.2 correction](research/service-launch-result.md)
- [Separate player lifetime: bounded worker preflight](research/service-lifetime.md)
- [Background player memory assessment](research/service-memory.md)
- [Bounded Spotify service playback 0.6-playback.1](research/service-playback.md)
- [Real backend runtime diagnostic 0.5-runtime.1](research/service-runtime.md)
- [Sources, provenance, and limitations](research/sources-and-licensing.md)
- [Configured native toolchain](research/toolchain-setup.md)
- [Executed validation](research/validation.md)

## Experiment handover

- [Scope and decisions](history/01-scope-and-related-work.md)
- [Environment and reproducibility](history/02-environment-and-builds.md)
- [Experiment ledger](history/03-experiment-ledger.md)
- [Technical findings and boundaries](history/04-technical-findings.md)
- [Current state and resume instructions](history/05-current-state-and-release.md)
- [Evidence and source map](history/06-evidence-and-sources.md)
