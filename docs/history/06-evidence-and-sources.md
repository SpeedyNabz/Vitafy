> Archived lab handover. Read [current status](../status.md) first; no historical VPKs or raw device logs are distributed here.

# Evidence and source map

All paths below are relative to this handover. Original frozen artifact verification
JSON describes the state at build time; later device observations live in the
separate device-evidence folders. Do not rewrite build evidence to pretend the
hardware test was known at packaging time.

## Current and historical records

| Record | Purpose |
| --- | --- |
| [Source lock](../../source-lock.json) | Source/dependency/toolchain pins; historical RC metadata plus current status |
| Current test (historical lab reference: `../artifacts/CURRENT_TEST.md`) | Current device instructions; mutable operational pointer |
| Hardware acceptance (historical lab reference: `../release/HARDWARE_TESTS.md`) | Scenario results; early banner/rows retain historical context |
| [Validation](../research/validation.md) | Host/native/device categories and executed checks |
| [Service playback](../research/service-playback.md) | Current pending real-audio test |
| [Service runtime](../research/service-runtime.md) | Real idle startup/shutdown result and memory caveats |
| [Service memory](../research/service-memory.md) | Static footprint, bounded allocations and budget reasoning |
| [Worker flag comparison](../research/service-close-worker.md) | Successful page-removal observation and partial-log limit |
| [Parent flag comparison](../research/service-close-parent.md) | Failed parent-only observation; raw-file archival limit |
| [Unflagged closure control](../research/service-close-control.md) | Page removal stops worker; shell normal |
| [Launch correction](../research/service-launch-result.md) | Argument 1 rejection and successful zero-argument evidence |
| [Lifetime design](../research/service-lifetime.md) | Pairing/worker boundaries and historical initial procedure |
| [Forced-close investigation](../research/forced-close-investigation.md) | Manual close versus game-close distinction |
| [Graphics gate](../research/home-lifecycle.md) | Isolated graphics gating implementation and tests |
| [Attribute control](../research/attribute-control.md) | Identical-executable metadata comparison; two-bit limitation |
| [Recovery](../research/recovery-and-shell-hang.md) | Shell regression/restart/recovery history |
| Release notices (historical lab reference: `../release/THIRD_PARTY_NOTICES.md`) | Existing dependency/asset notice work |

## Supplied device evidence

| Folder | Evidence retained |
| --- | --- |
| 0.2 stop (historical lab reference: `../artifacts/service-probes/device-evidence/probe02-bounded-stop/verification.json`) | Raw complete heartbeat, hashes, parsed 0..60 result and user shell confirmation |
| 0.3 memory (historical lab reference: `../artifacts/service-probes/device-evidence/probe03-memory1/verification.json`) | Raw launcher/memory/old heartbeat, parsed allocation result and shell report |
| 0.4 control (historical lab reference: `../artifacts/service-probes/device-evidence/probe04-close-control1/verification.json`) | Raw partial heartbeat/launcher/unrelated memory, later closure clarification |
| 0.4 parent (historical lab reference: `../artifacts/service-probes/device-evidence/probe04-close-parent1/observed-result.json`) | Extracted observations; raw Downloads files vanished before copy; later failure/shell report |
| 0.4 worker (historical lab reference: `../artifacts/service-probes/device-evidence/probe04-close-worker1/verification.json`) | Raw 0..9 heartbeat/launcher, hashes; later user report to 60 after closure |
| 0.5 runtime (historical lab reference: `../artifacts/service-probes/device-evidence/probe05-runtime1/verification.json`) | Numeric raw logs, hashes, parsed startup/shutdown/statistics and user shell confirmation |

Raw diagnostics remain outside the handover text archive. Sensitive account data
does not belong in documentation. 0.5 backend evidence includes a tightly filtered
derived log; no raw vam.log was copied. There is no 0.6 device result to catalogue.

## Implementation and checks

| Files | Role |
| --- | --- |
| Desktop test helper (historical lab reference: `../scripts/Test-Desktop.ps1`) / smoke (historical lab reference: `../scripts/Smoke-Preview.ps1`) | Mock contracts and UI smoke |
| Source preparation (historical lab reference: `../scripts/prepare_home_sources.py`) / Home tests (historical lab reference: `../scripts/Test-HomeLifecycle.ps1`) | Isolated frontend graphics overlay and desktop validation |
| Release packager (historical lab reference: `../scripts/package_release.py`) | Pin/patch reconstruction and native package checks |
| Backend audit (historical lab reference: `../scripts/audit_service_backend.py`) | Real static ELF load footprint without graphics |
| 0.1 source (historical lab reference: `../service-probe/worker.c`) / bounded core (historical lab reference: `../service-probe/worker_core.c`) | Original frozen tiny worker |
| 0.2 builder (historical lab reference: `../scripts/build_service_probe02.py`) | Correct only launch argument, preserve worker |
| Memory worker (historical lab reference: `../service-memory-probe/worker.c`) / core (historical lab reference: `../service-memory-probe/memory_core.c`) | Bounded allocation snapshots/cleanup |
| Close control builder (historical lab reference: `../scripts/build_service_close_control.py`) | Exact proven executable repackage |
| Parent builder (historical lab reference: `../scripts/build_service_close_persistence.py`) / worker builder (historical lab reference: `../scripts/build_service_close_worker.py`) | SFO-only flag placement comparisons |
| Runtime worker (historical lab reference: `../service-runtime-probe/worker.c`) / builder (historical lab reference: `../scripts/build_service_runtime_probe.py`) | Real idle runtime with audio denied |
| Playback worker (historical lab reference: `../service-playback-probe/worker.c`) / builder (historical lab reference: `../scripts/build_service_playback_probe.py`) | Bounded real audio trial, timeout report only |
| Playback checks (historical lab reference: `../tests/test_service_playback_probe.py`) | Actual package, native code/symbol, heap/SELF and frozen-source evidence |
| Playback verification (historical lab reference: `../artifacts/service-probes/VitaSpot-ServiceProbe-0.6-playback.1/verification.json`) | SDK/archive pin, native ELF/SELF/source hashes and package record |

Native build directories retain ELF/VELF and generated source evidence. Desktop
graphics-gate evidence lives under artifacts/home-lifecycle-desktop/. Original native
and release verification lives under artifacts/native/ and artifacts/releases/.
The generated package catalogue identifies actual current VPK files and hashes.

## External research provenance

The live detailed notes preserve original URLs, source dates and implementation
limits. References included VitaSpot upstream, VitaSDK/vdpm/vita-headers/vita-toolchain,
GrapheneCt's BG-App-PSV and Vita-PoC, a public shoui520/psp2 implementation reference,
and the SFO attribute research table. Source code can inform a hypothesis but is not
the user's firmware. The SFO table includes speculative entries. Device comparisons,
not the table alone, established the successful worker placement in this session.

No toolchain API/NID was invented to make the sample link. The GrapheneCt sample
uses notification-related attributes and discusses framebuffer limitations; this
project did not blindly copy those flags or install its suggested plugin. Branch
content fetched later is distinguished from pinned source claims in older notes.

The text snapshot preserves the exact authored research records, including
chronological pending statements and cited links. Current conclusions belong in
this index/ledger and later device evidence, not retroactive edits to frozen builds.
