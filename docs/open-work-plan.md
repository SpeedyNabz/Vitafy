# Open work plan

Updated **17 November 2026**. This is the living backlog and dependency order for
finishing Vitafy and pursuing playback across apps and games. An unchecked item
is open. Record a dated evidence link before closing it; compilation, mock state,
user reports and measured device results are separate evidence types.

## Baseline and target

Released: **0.1.0**, the unchanged user-accepted preview.7 Home binary, APP_VER
01.29. Development: **0.1.1-preview.1**, Home 01.31 / foreground configuration
01.32, ABI v6. The latter has 32 Rust tests, nine desktop tests, five UI suites
and native package/symbol checks; its Bluetooth and skip fixes await device
acceptance. See [current findings](research/current-findings.md),
[status](status.md) and [Bluetooth investigation](bluetooth-volume-and-skip-buffering.md).

Pinned inputs: VitaSpot `81c533f1ef61623360ea4fbe32cf29b4f2f3901f`, VitaSDK
`sdk-snapshot-20260825.611.1`, GCC 15.2.0, Rust nightly-2026-10-18. Historical
service probes used an older backend archive; their memory and ABI results must
not be applied unchanged to the current backend.

Target behavior: music continues while the UI returns Home, closes, reopens and
other supported apps/games run, with responsive controls, bounded resources,
correct routing and clean shutdown. **Playback across every app/game is an
aspiration, not an established Vita capability or a release promise.** A title
that claims exclusive resources may require an explicit compatibility limitation.
A background flag, BGM lease or successful idle worker alone proves none of this.

The current single-process player cannot keep executing after that process is
terminated. The preferred investigation is a small paired userland playback
worker with a separate UI. That architecture is a candidate requiring the gates
below, not a proven cross-game solution. A shell/kernel plugin is a separate
research decision if evidence shows userland cannot satisfy the target; none is
implemented or authorized for installation by this plan.

## Known evidence and blockers

| Area | What is established | What remains open |
| --- | --- | --- |
| Main app | Preview.7 Home generally accepted by user; 0.1.0 published | Later Bluetooth/skip defects, per-feature stress and recovery evidence |
| Bluetooth/Next patch | App gain separated from hardware levels; earlier one-song preload and async cancellation; host/native checks pass | Real volume/routing, skip latency, radio contention and underruns |
| Worker launch | Historical small probe launched with argument 0 and heartbeat after parent return | Current backend ownership and real audio lifetime |
| LiveArea closure | Unflagged and parent-only controls failed; worker-only candidate bit survived by user report | Current streaming worker and actual frontend detachment; no force-kill guarantee |
| Backend runtime | Historical worker initialized/discovered/shut down with audio blocked | Historical real-audio 0.6 test has no recorded result; modern backend audio untested there |
| Memory | Historical backend static load 15.16 MiB; idle sampled allocator about 1.03 MiB | Current static footprint, decoding peaks, granted worker budget and game contention |
| Games | Known game-triggered forced-close shell hang | Any safe active-audio game coexistence and recovery proof |
| Power | Home playback reported; foreground display timers implemented | Display-off, power-button behavior, true suspend and network resumption separately |

Do not repeat the known game-triggered forced-close transition to collect logs.
Close the current player through Settings before launching a game. Existing
historical probes are evidence references, not new instructions to install them.

## First work queue: app reliability

These tasks can progress before a service exists. Functional device results are
still needed even though the implementation and automated tests already pass.

- [ ] **A1 - Accept the Bluetooth/Next candidate (P0).** Test speaker, wired and
  Bluetooth connect/disconnect, physical buttons versus app/remote slider, mute,
  immediate and warmed Next, rapid skips, shuffle and queue edits. Compare 0.1.0
  and 0.1.1-preview.1 with the same track/network/quality. Record audible output,
  latency distributions and underruns, not only the UI buffering flag. Done when
  no app-induced level alternation or stale event regression is observed, and
  stalls have a classified recovery path. Preserve failure evidence before tuning.
- [ ] **A2 - Make remaining loading stalls observable (P0).** Time metadata, key,
  file start, cache hit, decoder-ready, output-ready and first audible sample where
  measurable. Count cancelled loads and underruns with rate-limited, account-free
  logs. Audit blocking reads separately from the 45-second async deadline; provide
  a responsive cancel/retry state instead of claiming that timer bounds all IO.
  Done when each reproduced stall is attributable and controls remain usable.
- [ ] **A3 - Playback/account recovery (P1).** Validate pairing/relaunch, expired
  sessions, Wi-Fi loss/rejoin, unavailable files, end-of-list/autoplay, remote
  handoff, seek, repeat and explicit queue edits. Keep account generations and
  authoritative heart acknowledgement/rollback. Verify likes both ways against
  Spotify desktop. Done with dated real-account results and explicit error paths.
- [ ] **A4 - Metadata/cache and large-order acceptance (P1).** Measure cold/warm
  launch, first usable rows, complete sort readiness, request counts, IO bytes and
  memory. Exercise 16/32 MiB pruning, full storage, corrupt/interrupted writes,
  account changes, revision changes and edit acknowledgement. Test 2,048/2,049,
  5,000/10,000 playable entries with duplicates/unavailable rows and explicit
  fallback above limits. Validate Next/Previous/window crossings/shuffle/repeat on
  device. Host limits are not yet measured safe Vita resource budgets.
- [ ] **A5 - Finish frontend acceptance (P1).** Touch/D-pad keyboard and sort
  dropdowns, library/sidebar/folders/pins, long Unicode titles, notifications,
  timer wake behavior and Settings Exit. Keep cached data labels and edit guards.
  Validate degraded network and allocation errors without UI freezes.
- [ ] **A6 - Candidate release decision (P1; after A1 and relevant regressions).**
  Preserve the accepted binary, lock/hash, debug files and evidence. Update
  release notes with remaining limits and dependency/font attribution. Publish a
  new release only from the accepted candidate; do not silently replace 0.1.0.

## Cross-app playback gates

Order: **B0 -> B1 -> B2 -> B3 -> B4 -> B5 -> B6**. IPC design may be prototyped
alongside early gates, but production integration does not bypass their evidence.
All engineering acceptance criteria below are proposed future requirements, not
measurements of the present build.

### B0 - Reproducible worker and feasibility baseline (P0)

- [ ] Inventory the historical launcher/worker sources, checks and hashes; move
  reviewed reproducible source into the standalone project, rather than depending
  on lab build folders or an old binary archive. Retain separate probe IDs/data.
- [ ] Record hardware model, exact firmware, installed plugin versions, account
  prerequisites and reference media behavior. The recorded model is PCH-1003;
  firmware/plugins are unspecified. Do not infer coverage for other configurations.
- [ ] Revalidate actual SDK declarations, link stubs and paired SFO/SELF metadata.
  Preserve evidence for the tested zero launch argument despite the misleading
  header comment; do not generalize that result across firmware without checks.
- [ ] Audit current ABI v6 backend static segments, module use, thread stacks,
  startup heap, lossless/other decoder working sets and one-next preload. Determine
  what background budget the device actually grants; archive/VPK size is not RAM.
- [ ] Define an allowed memory/CPU budget with a margin for the foreground title.
  Trim UI-detached browse snapshots/catalogues, artwork and discovery work; retain
  playback essentials. Do not copy the frontend's 48/64 MiB page targets into a
  worker or assume the old 48 MiB SELF request guarantees coexistence.

**Exit gate:** reproducible minimal package, verified current footprint and measured
budget headroom. If the worker cannot fit or obtain required resources, record the
constraint and choose a smaller service design before integrating the frontend.

### B1 - Real worker audio and controlled shutdown (P0)

- [ ] Reconcile the missing historical 0.6-playback.1 result if evidence exists;
  otherwise prepare a newly versioned bounded probe of the current backend. Never
  relabel old idle startup evidence as playback evidence.
- [ ] At Home only, demonstrate actual speaker/headphone audio, one skip, pairing,
  sufficient decode memory, and explicit pause/stop/shutdown while audio is active.
- [ ] Prove output-thread wake/drain/join, audio-port and BGM release, network/task
  teardown and ordinary app/shell health after stop. A watchdog report is failure
  evidence, not successful cleanup; do not force-exit an active audio owner.
- [ ] Check acquisition denial/busy outcomes and partial startup rollback. The
  installed SDK defines a BGM-busy error; the service must handle refusal cleanly.

**Exit gate:** audible playback and orderly active-audio stop, with stable sampled
resources. No game, power or page-removal transition is bundled into this gate.

### B2 - Streaming worker lifetime independent of UI (P0)

- [ ] Establish one owner of session, decoder, audio/BGM, saved state and credentials.
  The UI may attach/detach; it must not also open a second Spotify player/audio port.
- [ ] Verify current worker audio survives launcher return and normal UI closure,
  not just a tiny heartbeat surviving LiveArea removal. Reattach without restarting
  the song, duplicating the worker or replaying old mutations.
- [ ] Make worker lifecycle explicit: starting, running, detaching, draining,
  stopped, failed. Identify instances by generation/nonce, not a reused PID alone.
- [ ] Support bounded startup/stop acknowledgement and recovery from partial
  initialization, UI crash or missing worker. Cleanup must be owned by the worker;
  normal UI destructors cannot be relied on after the OS terminates that UI.

**Exit gate:** repeated safe detach/reattach and normal closures with uninterrupted
audio, no duplicate owner and clean final stop. The known forced-close test remains
excluded; the new design must remove dependence on the unsafe player process.

### B3 - Bounded IPC and frontend migration (P1)

- [ ] Spike an available SDK-supported local transport and measure latency/resource
  use. Choose it from actual declarations and tests; no unvalidated socket/API is
  assumed here. Define local access controls and credential/log exposure boundaries.
- [ ] Specify protocol version, framing/maximum bytes, command IDs, acknowledgements,
  instance/account generations, timeouts, event sequence numbers and reconnect
  snapshots. Reject truncated, oversized, unknown-version and stale messages.
- [ ] Serialize values only; keep raw C pointers, `vam_event_release` ownership and
  backend allocations in the owning worker. Stream page/image/ordered uploads in
  bounded chunks. Bound outstanding requests and event queues with backpressure.
- [ ] Define which commands are retry-safe. Never replay ambiguous playlist/save/
  queue mutations automatically; reconcile state before retry. Detach is distinct
  from stopping music, logout and terminating the worker.
- [ ] Preserve portable frontend/domain tests behind an adapter. Test missing/restarted
  worker, dropped connection, duplicate/out-of-order messages and account switches.
  UI rendering must not block on network/service replies.

**Exit gate:** real frontend controls and reconnects to the one streaming worker,
with equivalent hearts/queue/order behavior and bounded ownership/resource tests.

### B4 - Audio contention and resource coexistence (P1)

- [ ] Define policy for another music/media owner, a refused BGM lease, route changes,
  exclusive audio modes and unexpected port errors. Release or pause gracefully;
  never seize resources by repeatedly retrying or destabilize the shell.
- [ ] Measure active worker plus foreground UI/app resource use at Best, Medium and
  Low, including Bluetooth plus Wi-Fi. Verify early preloads do not starve current
  playback. Preserve explicit quality selection; any adaptive policy needs its own
  user-visible design and validation.
- [ ] Recheck foreground graphics/input gating and audio callback priorities when
  the service is active; no GPU resources belong in the worker. Measure scheduling
  and underruns rather than raising clocks/priorities speculatively.
- [ ] Verify lossless availability/fallback and audible output. Source preference
  does not make Vita's S16 PCM output bit-perfect 24-bit audio.

**Exit gate:** measured foreground/worker headroom, acceptable playback/control
latency and predictable contention handling in safe non-game scenarios.

### B5 - Staged app/game compatibility (P1; blocked on B0-B4)

- [ ] Define a recovery/rollback package and a test protocol for the new architecture
  that does not accept the known unsafe main-player forced-close prompt. Stop on
  a hang or shell corruption; investigate existing diagnostics rather than
  repeating the failing transition. If safe entry cannot be established, this
  gate stays blocked.
- [ ] Start with ordinary apps, then separate representative native games, demanding
  titles, homebrew, networked games and any supported compatibility environments.
  Record actual title/version/configuration; do not assume one category predicts
  another or that a title can coexist with a background app at all.
- [ ] For each supported entry, test launch, menus, gameplay, Home/return, UI reopen,
  game exit, worker stop and relaunch. Verify game audio alongside Vitafy, controls,
  route switching and shell health; keep credentials/private identifiers out of logs.
- [ ] Exercise repeated transitions and sustained playback on each supported route;
  agree latency/underrun/resource thresholds before declaring a pass. Count audible
  dropouts and recovery time, not only playing-state samples.
- [ ] Publish a supported/unsupported/untested matrix. If platform policy or a title
  blocks coexistence, investigate the constraint. Any plugin/system integration
  alternative requires a separate feasibility, install/recovery and compatibility
  plan; it is not an automatic continuation or a universal fix.

**Exit gate:** reproducible compatibility evidence for an explicit scope, no hangs,
no resource leaks and a documented fallback. An untested title stays untested.
A literal all-apps/all-games claim cannot be closed by sampling a few titles.

### B6 - Power, soak and release qualification (P1)

- [ ] Test screen dim/off, shell display-off, power-button entry and true suspend/
  resume as distinct states. Determine whether Wi-Fi/audio can run in each;
  graceful pause/resume is different from continuous playback through suspension.
- [ ] Measure battery/thermal behavior, network reconnection and route recovery.
  Include prolonged lossless sessions, repeated attach/detach, long queues, cache
  churn, low storage and allocation failures without polling/logging unboundedly.
- [ ] Define update/rollback and service-account-data migration while both UI and
  worker exist. Version protocol/schema changes; stop old instances safely before
  replacing executables. Do not uninstall/reset the user's account to update.
- [ ] Archive accepted sources, pins, hashes, symbols, host/native/device evidence,
  notices, known exclusions and recovery instructions. Resolve remaining upstream
  frontend/asset terms; publishing a binary did not resolve those rights.

**Exit gate:** supportable release within its tested compatibility matrix, with no
open P0 blockers. Current 0.1.0 acceptance does not close any service gate above.

## Remaining product features and deferred work

- [ ] **C1 (P2):** real account/device picker and clearer handoff/reconnect status,
  subject to supported service APIs and reliable ownership behavior.
- [ ] **C2 (P2):** cloud pin/custom-library-order sync only after authoritative data
  and mutation semantics are established; retain current local pins otherwise.
- [ ] **C3 (P2):** broader keyboard/input accessibility, locale-aware sorting and
  richer artist/credits/discovery data where supported and useful on 960x544.
- [ ] **C4 (deferred):** offline/download support requires entitlement, encryption,
  expiry/storage and explicit product design; existing audio cache is not downloads.
- [ ] **C5 (deferred):** video, social/collaborator features, Smart Shuffle/Mix and
  touring content require separate supported data/backend work. They do not block
  a focused reliable audio release and should not consume service headroom first.

## How to keep this plan open

For each task, add owner/branch, dependency, selected implementation, dated evidence,
measured result, unresolved failure and next action. Use open, in progress, blocked
(with reason), or done; never close a gate from a build result alone. No estimates
or device results have been invented. The immediate next evidence is **A1** for
the test build; the next service implementation work is **B0**, followed by **B1**.
