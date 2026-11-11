> Archived lab handover. Read [current status](../status.md) first; no historical VPKs or raw device logs are distributed here.

# Technical findings and boundaries

## Lifecycle and audio ownership

The original frontend is a normal game application. A game launch can close it;
streaming cannot continue inside a terminated process. The service direction puts
session/network/decoder/audio ownership in a paired background worker, with a future
frontend reconnecting through IPC. That architecture is being tested, not already
integrated into the production frontend.

Home.1 gates frontend graphics using installed `_sceAppMgrGetAppState` and
`sceCtrlPeekBufferPositive`, including system overlay/input-interception checks.
Unknown/error state denies graphics. While gated it still polls/releases backend
events and updates state/preferences; it skips GPU uploads/drawing/present, deferred
texture destruction, UI input and frontend brightness/dimming policy. It rechecks
state before presenting and breaks immediately on quit. Rust streaming/audio threads
are independent. There is still a sampling-to-GPU-submission race; no atomic
ownership guarantee was proved.

Rust's Vita sink acquires an AppMgr BGM lease around a BGM-type sceAudioOut port.
Home.2's explicit removal of ownership stopped Home audio, so removing the lease is
not a valid feature-preserving fix. This does **not** prove the lease caused the
game-close hang. Successful manual closure and failing game-forced closure differ.

Sink cleanup signals its ring, joins the output thread, then releases audio port
and lease; Drop calls stop. A blocked output call could affect that order, but it is
not an established root cause. Forced process termination may not run Rust Drop.
Do not infer destructor behavior from a normal-return test.

## SFO attributes and SELF boot parameters are distinct

| Field/bit | Observed use/limit |
| --- | --- |
| SFO ATTRIBUTE 0x8000 | Working baseline communication-zone setting |
| SFO bit 0x8 | No-suspend-on-deactivation candidate; home.1 preserves 0x8000 and adds it |
| SFO bit 0x80000 | Disable-system-suspend candidate used in withdrawn RC1; no sleep success established |
| SFO bit 0x04000000 | Research-table persistence candidate; parent-only fails, worker-only passes LiveArea removal by user report |
| SELF Control Info ATTRIBUTE 3 | Applied to service worker only; not the same as SFO ATTRIBUTE |
| SELF MEMSIZE | VitaSDK value is KiB; 4096=4 MiB, 24576=24 MiB, 49152=48 MiB |

Attribute-control changed 0x8 to 0x8000 with identical executable: it both removed
one bit and restored another. That field-level comparison does not isolate the
single causal bit behind the older shell regression. Do not restate it as proof
that bit 3 alone is broken.

Installed SELF tooling accepts system-app requests in the documented tool range
0x1000..0x12800 KiB (4..74 MiB). Tool acceptance is not a firmware allocation guarantee
or game coexistence test. Newlib's default heap request is 128 MiB unless a strong
`_newlib_heap_size_user` overrides it. Inspect the actual ELF DWORD, not only a source
constant. The diagnostic heap can grow its arena lazily, so distinguish configured
capacity, arena size, allocated/free chunks and kernel pool statistics.

## BG_APP_UTIL launch argument discrepancy

Installed bgapputil.h says mode 1, but the device rejected 1 with 0x80106501 and no
heartbeat. GrapheneCt's native caller uses 0; an inspected public implementation
reference rejects nonzero flags. Passing 0 made the same worker launch and advance
on the user's device. The SDK header itself was preserved. The installed header's
error enum also disagreed with the earlier native reference's invalid-argument value.

The public implementation is not a dump of this user's firmware. Recorded sample
links/commit strings are in the supporting research notes. Some pinned raw URLs failed
to fetch during later browsing; a readable branch example alone does not independently
revalidate a commit pin. No NID was invented and no kernel/shell plugin was installed.
Use installed weak BG_APP_UTIL stubs because the launcher loads that module in main.

## Paired service packaging

Parent gdc title VTSV01001 has `PUBTOOLINFO=has_bg=1`. Worker gdd VTSV01002 is
`eboot2.bin` with `sce_sys/param2.sfo`; its install/save directory references point
to VTSV01001. The launcher is eboot.bin. This pairing is separate from VTAM00001.
Current service data root is `ux0:data/VitaSpot-ServiceProbe/`.

Startup return to Home is not the same as peeling/removing the LiveArea page. The
unflagged and parent-only probes stopped progressing after removal; worker-only
0x04008000 kept progressing through sample 60 by user report. That is a page-removal
test, not killing an active graphical frontend or accepting a game's force-close
prompt. Persistence still needs validating with active audio and later the actual UI.

## Memory and runtime

An audit retained all 25 C ABI entry points in a real linked ELF without SDL/GXM.
Its PT_LOAD memory sizes total 15,893,348 bytes. It cannot fit the tiny 4 MiB probe
configuration. This is not a minimum for an optimized/reduced backend, nor the disk
archive/VPK size. Runtime allocations, stacks and loader/module overhead add to it.

Inspected runtime source requests a 1 MiB network pool, one 1 MiB runtime thread
stack and two 512 KiB Tokio worker stacks. Image/audio threads have separate stack
requests when used. A 48 MiB SELF/24 MiB heap subsequently booted and completed idle
runtime initialization/shutdown. Actual allocated/granted physical memory, TLS and
decoder peaks, long-playback growth and game resource sharing remain unmeasured.

Runtime probe 0.5 blocks actual audio-open/BGM-acquire calls with wrappers returning
-1. Its normal logged-out run reaches discovery and then shutdown without pairing.
Playback 0.6 removes those denials and uses actual SDK imports. Both create fresh
per-PID data directories and refuse reuse; they do not load the main VitaSpot account.

0.5 has a timeout that can exit the probe process when audio is blocked. **Do not
carry that forced-exit behavior into a live audio owner.** 0.6's monitor only records
a timeout; its normal path requests pause and backend shutdown. A timeout report
does not establish cleanup or a hard real-time bound.

## IPC and future frontend integration

The current frontend C ABI is process-local. Event/page/image/lyrics pointers are
backend-owned and must be released by vam_event_release. They cannot be passed as
raw addresses between processes. A future transport must serialize bounded data,
validate lengths/version/request IDs, carry commands/status and keep ownership in
the worker. Frontend disappearance should not destroy playback; frontend return
must reconnect, resynchronize and handle service restart without duplicate sessions.
No such integrated frontend transport is implemented yet.

Dormant newlib file-I/O dispatch can link sceNetSend/Recv/SocketClose even in a
file-only probe. Presence of those symbols alone does not prove network setup.
Conversely the real backend carries network/audio imports even when linked inactive.
Describe actual initialization and calls, not a misleading global "no network symbols"
claim.
