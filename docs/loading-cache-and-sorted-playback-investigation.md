# Loading, local metadata and sorted playback investigation

The recommendations are implemented in **preview.7**; see the
[implementation, bounds and acceptance report](releases/preview7.md). The analysis
below preserves the preview.6 baseline and the alternatives considered. Its
statements about missing caches and the 2048 limit describe preview.6, not the
current implementation. Physical-device timings, audio continuity and memory-card
profiling remain pending.

## Pinned baseline

VitaSpot `81c533f1ef61623360ea4fbe32cf29b4f2f3901f`; VitaSDK
`sdk-snapshot-20260825.611.1`; GCC 15.2.0; Rust nightly-2026-10-18; backend ABI v5.
Dependencies remain pinned in the source lock. Findings below come from local
declarations and call sites, not assumed Spotify service limits.

## What is already stored locally

| Data | Current storage | Consequence |
| --- | --- | --- |
| Streaming audio | `ux0:data/VitaSpot/cache/audio/`, bounded by the selected 256/512/768/1024 MB preference | Increasing this helps audio reuse, not playlist metadata. It is not an offline-download feature |
| Encoded artwork | `cache/img/`, backend `ByteCache`, 30 MiB cap with eviction | Survives relaunch. Index scanning runs on a worker in the normal path; thread creation failure falls back to synchronous scanning |
| Decoded artwork | Frontend texture cache, 96 entries, at most six image requests in flight | Disk cache hits still require reading and decoding; raising texture count costs graphics memory |
| Track/episode metadata | `MetaCache`, RAM only, maximum 2048 items, clears the entire map on insertion at capacity | Large libraries can repeatedly lose hot metadata. Cold relaunch resolves names/albums/dates again |
| Library and saved state | RAM `SavedCache`, nominal 15-second refresh interval, serialised reads/writes | Relaunch has no persisted browse snapshot. Collection/rootlist fetches are coupled to library refresh |
| Display preferences and pins | Local settings and 64 account/URI preference records | Already persistent; these contain presentation state, not a searchable music catalogue |

Sources: [backend paths](../build/source/backend/librespot-vam/src/state.rs),
[shared caches](../build/source/backend/librespot-vam/src/shared.rs),
[image disk cache](../build/source/backend/librespot-vam/src/images.rs),
[frontend images](../build/source/src/gfx/images.c),
[metadata](../overlays/backend/librespot-vam/src/metadata.rs),
[saved state](../overlays/backend/librespot-vam/src/saved.rs),
[preferences](../src/prefs.c). Generated-source links describe this reconstructed
checkout; implementation must edit canonical sources/overlays, not `build/source`.

## Likely sources of loading work

1. [Playlist page construction](../overlays/backend/librespot-vam/src/pages.rs)
   calls `internal::playlist` before taking the requested 50-entry window.
   [That function](../overlays/backend/librespot-vam/src/internal.rs) fetches the
   playlist and follows service pagination until complete. There is no shared
   playlist-snapshot cache at this call site. Automatic full loading repeats this
   path for each frontend slice. For a 5,000-entry example, there are 100 UI
   slices, each requesting the complete snapshot again. This is a call-count
   illustration, not measured traffic or a claim that the transport never reuses data.
2. `MetaCache::put` clears all entries at the 2048-item threshold rather than
   evicting the least useful item. `resolve_playables` also treats missing album
   title or release date as a miss, even when the service genuinely omitted it.
   Unknown data can therefore cause repeated requests. Duplicate missing URIs
   are not deduplicated before the batch lookup.
3. The library refresh fetches collection sets and rootlist together. Heart
   reconciliation, browsing and rootlist discovery have different freshness needs,
   but currently share the same broad refresh machinery and serialization gate.
4. Frontend pages copy fixed-size `vam_item` records into growable arrays; retained
   navigation views and Rust items add further copies. Complete loading increases
   RAM use even when only a few rows are visible. Do not respond by retaining every
   artwork texture or duplicating every full page in a disk format.

These are code-supported optimization candidates. First measure which dominates
on the actual Vita, including TLS/network latency, image decode and memory-card IO.

## Recommended cache design

### First: reuse one playlist snapshot while paging

Add an account/session-scoped snapshot store keyed by playlist URI, with revision,
ordered entry IDs/URIs, added dates and collection metadata. A single-flight fetch
lets parallel page requests share one operation. Each 50-entry reply slices the
same immutable revision instead of downloading the complete list again.

Hold the snapshot through an active load. Revalidate in the background when
reopening, after an edit acknowledgement, or on a relevant remote change. Reject
stale positional edits exactly as now. Do not merge old and new revisions or
replace a partially loaded playlist. Keep epoch/account checks before publishing.
Bound the store by bytes as well as playlist count and pin only active snapshots.

### Second: persist compact browse metadata

Start with a versioned local store under `cache/metadata/<account-key>/`, using
the existing serialization dependencies rather than adding a database immediately.
Separate records into:

- Track core: URI, title, artist/album IDs and names, duration, release date,
  artwork key, field-presence/completeness flags, fetched time and schema version.
- Playlist snapshot: URI, revision, owner and an ordered vector of entry identities
  and added dates. Duplicate tracks must remain separate playlist entries.
- Root library summary: folders/parents, names, ownership and last verified time.

Render a validated cached snapshot immediately, label stale data where needed,
then reconcile asynchronously. Use atomic temporary-file replacement, bounded
lengths/counts, corruption checks and a recoverable index. Perform reads/writes
and pruning on workers; never scan thousands of files or serialize a full library
on the render/audio thread. Prefer batched records or a small index over one
tiny file per track. Coalesce writes and update only changed records.

Prototype a configurable byte budget (for example 16–32 MiB) and measure it;
this is a proposed budget, not a measured safe device limit. Evict cold records
incrementally. Preserve recent/active snapshots and reuse current image/audio caches.
Avoid caching a whole expanded C ABI struct; store compact fields and hydrate only
visible rows. Consider SQLite only if profiling shows indexed lookups/transactions
justify the new dependency and Vita IO behavior is validated.

### Freshness and account correctness

Use stale-while-revalidate for names/artwork and shorter refresh intervals for
membership/ownership. Store explicit known-unknown metadata, with a bounded retry
time; do not permanently turn network failures into unavailable tracks. Deduplicate
missing track IDs before network batches. Replace full-map clears with bounded LRU
or clock eviction, keeping lookup behavior predictable.

Keep authoritative saved-state reconciliation and pending-heart rollback rules.
A disk snapshot must not overwrite a later acknowledged save, restore a pending
old-account write, or silently authorize an edit. Namespace by account, keep
tokens out of the metadata store, clear account-specific RAM on logout and define
an explicit disk-retention/clear-cache policy. Keep private cache files out of Git
and packages. Persist metadata for browsing; do not imply offline playback rights.

## Why sorted playback stops at 2048

The immediate failure is in [app_play_ordered](../src/app.c): after requiring a
complete snapshot, it rejects `count > 2048`. The real backend's
[vam_play_ordered](../overlays/backend/librespot-vam/src/ffi.rs) independently
rejects the same count. Removing only the toast/first check leaves the backend
rejection. Counting is currently based on all loaded entries before unavailable
rows are removed, which can reject a list even if fewer than 2048 are playable.

The command copies URIs into Rust strings and
[loads an explicit track context](../overlays/backend/librespot-vam/src/engine.rs).
The pinned [Spirc adapter](../overlays/backend/librespot-vam/vendor/librespot-connect/src/spirc.rs)
creates a `ContextPage` from that vector. These call sites do not establish a
Spotify-imposed 2048 maximum; the number is an app/backend guard. This also has
costs beyond a C array: cloned strings, protobuf/context entries, Connect state,
queue metadata and subsequent notifications. The mock backend has its own
smaller `MAX_CTX=512`; current 320-row UI tests cannot prove larger playback works.

## Possible fixes and tradeoffs

| Option | Benefit | Work and risk |
| --- | --- | --- |
| Count playable entries before enforcing a limit | Stops false rejections caused by unavailable entries | Preserve original entry identity and selected-index mapping; does not solve genuinely large contexts |
| Raise a shared, explicit limit with byte accounting | Smallest end-to-end fix for ordinary large playlists | Change frontend, FFI, mock and tests together. Validate allocations, Connect serialization, Next/Previous, shuffle, repeat and handoff on hardware. A larger constant alone is not proof of safety |
| Upload ordered IDs in chunks and retain a backend order token | Avoids one giant FFI handoff and duplicate frontend URI arrays | New bounded begin/append/commit/cancel protocol, account/revision binding, cleanup on timeout; backend still needs a playback strategy |
| Backend-owned ordered context with a rolling playback window | Best bounded-memory approach for very large lists | Maintain global order/selected offset and refill around boundaries. Preserve previous history, duplicates, shuffle/repeat and external queue edits; prevent audible reloads or losing Connect state. Requires a tested Spirc/context extension, not replacing the context on every Next |
| Offer normal Spotify context playback as an explicit fallback | User can play immediately when custom sorted playback cannot be prepared | Must clearly ask/label that subsequent songs follow original playlist order. Never silently claim the selected sort is honored |

Recommended sequence: fix playable-count mapping and add real >2048 fixtures;
profile a modest higher shared limit as an interim solution; move the full order
to a backend-owned snapshot/token and evaluate rolling windows for bounded memory.
Do not remove all caps without byte budgets and failure handling. No playlist
mutation should be used merely to obtain sorted playback.

## Measurements and acceptance before implementation

- Record cold launch, warm launch and reopened-playlist time to first usable rows,
  time to full sort readiness, playlist/metadata request counts, cache hit rate,
  decoded image count, peak RAM and metadata IO bytes. Report real device results
  separately from host/mocks; scrub private IDs from shared evidence.
- Use small and large synthetic fixtures around boundaries: 2048, 2049, 5000 and
  a larger stress case, including duplicate entries and unavailable songs. Count
  playable records independently. Raise mock limits with allocation checks or add
  a dedicated real adapter harness; do not mistake the 512-entry mock for coverage.
- Test interrupted/partial/corrupt metadata writes, cache eviction, full storage,
  account switching, server revision changes mid-load and edit acknowledgements.
- For ordered playback, start at beginning/middle/end; exercise Next/Previous,
  shuffle/repeat, queue edits, window crossings and Connect handoff. Verify actual
  order and audio continuity on the Vita before declaring the limit fixed.

Suggested milestones: (1) instrument; (2) snapshot reuse and incremental RAM
eviction; (3) compact persistent metadata with revalidation; (4) larger-context
playback strategy and hardware soak. Preview.6 changes only the requested UI;
the sorted playback limit remains present until that follow-up is implemented.

## Current follow-up, 17 November 2026

The metadata-cache plan is implemented; it is not evidence that audio Next has
no network latency. Development 0.1.1-preview.1 separately changes next-song
preloading/cancellation; see [the audio investigation](bluetooth-volume-and-skip-buffering.md).
Hardware cache/order measurements and worker-specific resource budgets remain
open in [A4 and B0](open-work-plan.md). Old 15.16 MiB static-service measurements
must not be reused as the current backend's runtime memory budget.
