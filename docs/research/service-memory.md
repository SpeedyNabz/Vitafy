> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

# Background player memory assessment

9 November 2026. Service Probe 0.2 passed its initial device preflight: accepted
launch, advancing heartbeat after launcher return, all samples 0..60, and the
intended limit-stop record after 120.313727 seconds. The user subsequently
confirmed everything works correctly, completing the requested post-stop Home
folder/ordinary-app check. This is not an independent process-exit measurement.

## Actual backend link audit

`py scripts/audit_service_backend.py` links the existing real Rust archive into
an ARM ELF without frontend, SDL, GXM or display dependencies. All 25 public
backend API entry points are retained and verified in the linked symbol table.
The program does not call those functions. No VPK was produced or run on Vita.

| ELF load segment | Memory bytes |
| --- | ---: |
| Read/execute | 14,997,108 |
| Read/write, including BSS | 896,240 |
| Total | 15,893,348 (15.16 MiB) |
| Existing probe SELF budget | 4,194,304 (4 MiB) |

This measured link cannot fit the probe's 4 MiB request. It is not a theoretical
minimum for an optimized or reduced backend. Archive size and compressed VPK
size are not runtime memory measurements. The static total excludes runtime
heap, thread stacks, module overhead, decoder buffers and network allocations;
the current network source alone requests a 1 MiB pool. The audit's explicit
256 KiB newlib heap is only a link-only placeholder, not a usable player budget.

The exact backend archive SHA-256 is
71a7f505f9087e90b5b39b703251c9f4be6270771a9449621328e1ab018de5f0,
matching the archive used by home.1. SDK snapshot is
sdk-snapshot-20260825.611.1. Full segment, input and ELF hashes are retained in
static-load.json (historical lab reference: `../artifacts/service-probes/backend-memory-audit/static-load.json`).
The ELF, generated harness and native linker log are in build/service-backend-audit/.

## Prepared device diagnostic

Service Probe 0.3-memory.1 (historical lab reference: `../artifacts/service-probes/VitaSpot-ServiceProbe-0.3-memory.1/VitaSpot-ServiceProbe-0.3-memory.1.vpk`) is built and its device logs now confirm successful launch and the intended memory-test stop path. Its worker requests 24 MiB, with 4 MiB heap and the real backend retained but uncalled. It allocates at most two 1 MiB blocks, touches their pages and frees them, including on allocation/logging failures. Host failure/cleanup cases and three native/package tests passed. Sources, native evidence and the precise test procedure are retained in the artifact README (historical lab reference: `../artifacts/service-probes/VitaSpot-ServiceProbe-0.3-memory.1/README.md`). This checks one candidate budget, not the maximum gdd limit or real playback headroom. It makes no frontend-closure flag changes.

## Device result: 0.3-memory.1

The supplied launcher.log identifies probe=3, launcher PID 12398267, load_bg_app_util=0 and start_bg_app=0. memory.log identifies worker PID 6826821 and the configured 24 MiB budget/4 MiB heap. Both page-touched 1 MiB allocations succeeded, followed by a released snapshot, ten heartbeat records and status=stopped reason=bounded_memory_probe allocation_result=0. All 14 memory snapshots returned mem_rc=0 and reported user=2097152, cdram=0, phycont=0. The unchanged pool snapshot is consistent with allocations inside the reserved newlib heap; it is not a measurement of free malloc space or the player's peak demand. This demonstrates boot acceptance with the candidate budget and successful bounded allocations, not an exact physical grant or sufficient playback headroom.

The accompanying heartbeat.log is the old 0.2 log under PID 9055701; it must not be attributed to this run. Original files, hashes and parsed checks are retained in artifacts/service-probes/device-evidence/probe03-memory1/. The user confirms post-stop shell health is normal for 0.3; the initial memory diagnostic is complete. No independent process-exit measurement is claimed.

## Next implementation gate

The bounded memory diagnostic has now reached its intended stop on device. Confirm post-stop shell health, then establish explicit frontend-closure behavior separately. Backend initialization and runtime heap/stack measurement are still required before playback migration. For future diagnostics, keep
the proven zero launch argument, separate title IDs/data directory, no audio,
and no account-file access. Retain the real backend's static footprint while
leaving its functions uncalled. Record startup, heap configuration and free
memory using the installed SDK declaration of sceKernelGetFreeMemorySize and
SceKernelFreeMemorySizeInfo. Its three memory-pool values are snapshots, not
proof of a private per-process allocation limit.

Determine the accepted gdd budget on the device; a SELF generator accepting a
larger MEMSIZE does not prove firmware will grant it or coexist with games.
Use bounded, explicitly freed allocation steps with checked failure handling,
not an unbounded allocation-to-exhaustion loop. Preserve baseline probe and
device logs. Do not add BGM ownership, networking or a forced game closure to
this memory check. Only after an acceptable budget is demonstrated should the
worker initialize the backend and measure its real startup/playback high water.

Explicit frontend closure is another pending gate. The current launcher returns
to Home; that does not establish worker survival across LiveArea removal. Its
test must remain separate from the memory change. Later frontend/service IPC
must serialize bounded data and release backend-owned events in the worker;
C ABI pointers cannot be transferred between processes.

Game coexistence, forced-close handling, Spotify worker playback and power-button
behavior remain unverified. No new device package is recommended by this audit.
