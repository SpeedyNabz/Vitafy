# Preview.7 - local browsing and larger sorted playback

Implemented from the [loading investigation](../loading-cache-and-sorted-playback-investigation.md).
Home package **01.29**, ATTRIBUTE=32776, title ID `VTAM00001`. This updates the
existing player and retains `ux0:data/VitaSpot/`. No installation was performed.
The foreground build configuration is 01.30; only the Home package was built here.

## Implementation against the recommendations

| Recommendation | Implementation |
| --- | --- |
| Reuse playlist snapshots | Account/session-scoped immutable snapshots, per-URI single-flight gates and consistent revision/offset checks. All 50-row slices reuse the snapshot. Reopening revalidates after 60 seconds; edits invalidate RAM and remote library events refresh affected views. |
| Bound snapshot RAM | At most 16 snapshots and an estimated 8 MiB, with 30-second paging pins. Pressure can evict a new snapshot when all others are pinned; this bounds retention but may require another fetch. |
| Persist compact browse data | Versioned account-scoped JSON records for compact metadata, playlist entry identities/revisions and library summaries. Workers perform IO, serialization and pruning. Existing serialization dependencies are reused; no database dependency was added. |
| Recover interrupted writes | Checksums, schema/account validation, length/age checks, synchronized temporary writes and recoverable previous-generation backups. Incomplete temporary records are ignored. Reads accept records up to seven days old; cached pages are labelled while background reconciliation runs. |
| Bound disk usage | Settings select a 16 or 32 MiB per-account budget (default 32), with at most 64 records. Cold playlist records are evicted; metadata/library records are protected. Individual files over 16 MiB are rejected; writes over 8 MiB are skipped. Coalescing avoids unchanged rewrites; the compact track catalogue is capped at 16,384 entries and 6 MiB JSON. These are engineering bounds, not measured safe hardware budgets. |
| Preserve account and heart correctness | Epoch/generation checks before publishing and committing IO; logout deactivates old work before purging that account's metadata. Cached library summaries never seed authoritative saved sets. Stale ownership cannot authorize edit controls. Pending-heart acknowledgement, rollback and live reconciliation remain. |
| Clear policy | Settings offers acknowledged Clear browse cache, invalidating current-account RAM and disk metadata. Credentials, audio and image caches are separate. Purge failures are reported/logged. No offline-playback entitlement is implied. |
| Avoid metadata churn | Deduplicated network batches, incremental LRU with 16,384 entries and an estimated 8 MiB ceiling. Known absent fields are cached; incomplete fields retry after one hour, complete metadata after 24 hours. Failed refresh retains last good metadata without caching network failure as unavailable. |
| Separate freshness needs | Saved-state reconciliation retains its 15-second cadence; root library refresh uses five minutes unless an edit invalidates it. Recent contexts reuse a 60-second cache. Cached Home/library rows avoid blocking on fresh details. |
| Bound frontend retention | A page allocation is capped at 48 MiB. Inactive navigation page copies are evicted toward a 64 MiB total-page target; active/library pages are protected. Navigation state survives eviction and reloads on revisit. This is not a cap on total process memory. |
| Correct playable counting | Unavailable entries are excluded before applying the ordered-playback limit. Selection maps by original entry position, preserving duplicates. |
| Larger ordered contexts | Shared limit of 10,000 playable tracks and a 1 MiB URI/bookkeeping budget. Begin/append/commit/cancel protocol uploads at most 128 IDs per frontend frame without a giant frontend URI array. Partial, expired (30 seconds), wrong-session and cancelled uploads cannot commit. Navigation/refresh cancels preparation. Allocation failures remain explicit. |
| Rolling playback windows | Inspection found the pinned Connect adapter already maintains up to 80 upcoming and 10 previous tracks. The full order now belongs to that backend context. The existing window machinery is retained and tested, rather than replacing the context on each Next. The complete ordered context still uses memory proportional to playlist size; the 10,000-track cap remains deliberate. |
| Explicit fallback | Failed preparation or over-limit lists expose a labelled original-order playback action. It uses the original playlist context and offset only when selected; the app never silently changes the requested sort. |
| Instrumentation | Page elapsed time/rows/offset, metadata hits/misses/batches/estimated bytes, snapshot/disk hits and IO bytes. Foreground resource samples every five seconds report C page allocations, decoded texture count/estimated bytes and SDK free user/CDRAM/physical memory through the backend logger. Minimum sampled free user RAM is not true peak RSS. |

SQLite remains a conditional alternative, requiring profiling to justify a new
dependency. It is not needed for this bounded initial store. Audio/image caches
and decoded-artwork limits remain separate. Home lifecycle behavior, lossless-first
Best quality, Medium/Low options, Never-default timers, shared likes and the
full-screen keyboard are retained.

## Validation and limits

- **28 real Rust tests passed**, including cache corruption/schema/account/expiry,
  backup recovery, failed writes, budgets/coalescing, old-session rejection,
  metadata LRU/unknown fields and immutable 5,000-entry snapshot reuse.
- **Nine Windows CTest cases passed**, including the actual upload ABI at 2,048,
  2,049, 5,000 and 10,000 entries with duplicate positions, cancellation and
  incomplete/oversized input. **Five offline UI suites passed**, including full
  5,000-song loading, sorted chunk upload and explicit fallback on injected failure.
- Real pinned Connect protobuf roundtrips preserve 5,000/10,000-entry order.
  A host harness exercises the actual rolling-window state through 170 Next
  transitions, Previous, and repeat wrapping; the queue remains within 80/10 bounds.
  These tests do not prove audible continuity or remote Connect behavior.
- Fresh native Home cross-build and package verification passed: ABI v6, SFO,
  CRC/SELF, artwork, required symbols/imports and hashes. Debug information is
  retained; readelf confirmed `.debug_info`, `.debug_line` and `.symtab`.
- SDK and upstream unchanged: VitaSpot `81c533f1ef61623360ea4fbe32cf29b4f2f3901f`,
  VitaSDK `sdk-snapshot-20260825.611.1`, GCC 15.2.0, Rust nightly-2026-10-18.

Evidence: [host](../evidence/preview7-host.json),
[native Home](../evidence/preview7-home-native.json).

## Physical-device acceptance still required

Measure cold launch, warm relaunch and playlist reopen: first usable rows, complete
sort readiness, requests/hit rate, IO bytes, decoded textures and RAM samples.
Compare the same account/playlist/network conditions with the previous build;
report numeric evidence without private IDs. No speedup or RAM safety measurement
has been claimed from host mocks.

On a Vita, exercise low/full storage and power-interrupted writes, account switching,
remote revision changes during load, edit acknowledgement, and 16/32 MiB eviction.
Test beginning/middle/end of large lists, unavailable rows and duplicates, window
crossings, shuffle/repeat, queue edits, remote handoff and sustained audio. Check
Home/return and orderly Exit separately. Actual service persistence, latency,
flash durability, audio continuity, host like synchronization and long-run memory
behavior remain pending; host fixtures cannot close those checks.

Preview.3 Home playback remains confirmed by the user's earlier report. Do not
repeat the known unsafe game-triggered forced-close transition; exit Vitafy before
launching a game.

## Release promotion

The user subsequently confirmed this build works as intended. It is promoted
unchanged to [release 0.1.0](0.1.0.md); the pending measurements above are not
implied by this general acceptance report.
