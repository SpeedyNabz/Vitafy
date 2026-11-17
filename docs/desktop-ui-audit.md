# Spotify desktop UI audit and implementation status

Updated 17 November 2026: released **0.1.0**, development **0.1.1-preview.1**,
backend ABI v6. The original 15 November desktop
comparison used the user's open Spotify Windows app and supplied screenshot.
The user subsequently accepted the preview.7 Home build for release 0.1.0;
see the [device report](evidence/preview7-home-user-report.json). Builds/mocks
and specific unmeasured account/device checks retain separate evidence.

Baseline remains VitaSpot `81c533f1ef61623360ea4fbe32cf29b4f2f3901f`, VitaSDK
`sdk-snapshot-20260825.611.1`, GCC 15.2.0 and Rust nightly-2026-10-18. Upstream
and dependency revisions remain pinned; canonical overlays and their hashes are
in `source-lock.json`. ABI v6 requires the freshly built backend. The historical
ABI v3 audio archive is rejected by the current build script.

The [open work plan](open-work-plan.md) tracks the remaining UI/backend/device
work. The [Bluetooth/skip candidate](bluetooth-volume-and-skip-buffering.md) has
host/native evidence but no new user device acceptance. General acceptance of
0.1.0 does not replace the specific live-account/stress checks listed below.

## Implemented

| Area | Current implementation | Bounds and acceptance |
| --- | --- | --- |
| Candidate volume/preload | 0.1.1-preview.1 uses independent app gain, one earlier next-song preload, obsolete async-load cancellation and stale-event guards | Physical buttons control hardware levels separately. Bluetooth, audible latency and underrun tests pending; 45-second async deadline does not bound blocking IO |
| Persistent library sidebar | Artwork, title/owner, selected playlist, playing indicator, context menu, Liked Songs, retry and paged library | Actual-account touch/D-pad acceptance pending |
| Playlist track sorts | Dropdown with Custom order, Title, Artist, Album, Recently added, Release date, Duration; select active sort to reverse direction | Missing dates remain last; stable ties preserve duplicate-entry offsets. ASCII case folding preserves UTF-8 but does not reproduce Spotify's language-specific collation |
| Metadata | Album name, album release date, per-entry added timestamp, entry ID, original index and snapshot revision | Unknown source fields stay unknown; no URI-as-name or position-as-date substitutions |
| Complete loading | Automatically load all playlist and Liked Songs pages without scrolling or Load all; show progress and retry only after failure | No 2048-song display cutoff. Changed revisions, wrong offsets and allocation failures are errors; never label partial results complete |
| Sorted playback | Complete display order is copied into an explicit ordered playback request; duplicate tracks start by index | Requires a complete snapshot; cancellable chunked ordered playback supports up to 10,000 playable songs with a 1 MiB URI/bookkeeping budget and explicit original-order fallback. Subsequent playback uses the generated queue, rather than reloading the unsorted Spotify context. Connect labels may describe that generated context; on-device Next/Previous/shuffle acceptance remains pending |
| Library organisation | Library order, Recents, Recently added, Alphabetical, Creator dropdown; category chips combine with By you; simple list rows and right-aligned sorting dropdown | Recents ranks the fetched recent-context history (up to 50), leaving unknown entries last. Library order is the actual rootlist order, not an invented play history |
| Search | Global search with full-screen Vita keyboard; text filters removed from library and playlists | Category/By you chips remain; Downloaded removed; no offline-download store exists |
| Presentation persistence | Sort/direction keyed by account and URI; library/playlist presentation fixed to list | Bounded to 64 records; no credentials or tokens stored in these records |
| Navigation | Bounded back/forward history with retained page, scroll, filters and focus | New branches discard forward views; account changes clear history |
| Status | Persistent local/remote device, buffering, paused and known Wi-Fi-loss labels; refresh/retry | Real network-loss and Connect handoff acceptance pending |
| Song details | Album/release year and added date in context sheet, album/artist navigation, existing lyrics and queue shortcuts | Artist biography/credits require a separate data source |
| Playlist edits | New private playlist, owned-playlist rename, Add to playlist picker, revision-guarded single-entry remove/up/down | Async acknowledgement, one frontend edit at a time, ownership validation and session-epoch isolation. Spotify scope denial/rate limits/errors remain explicit failures. No success message before acknowledgement |
| Folders | Preserve rootlist group markers and parent links; read-only folder/subfolder navigation, including empty folders | Up to 16 levels. Folder art toggles collapsed descendants in the sidebar; pinned children remain visible. Rootlist fetch is paged and limited to 5000 entries; truncation/conflicts produce an error rather than a false complete result |
| Pins | Pin/unpin playlists and folders on this Vita, ordered before other sidebar entries | Per-account local preferences; no cloud pin claim |
| Queue editing | Distinct playing/explicit queue/context sections; remove/up/down and confirmed clear of explicit queue | Revision guard and local active device required. Context songs and duplicate identities retained |
| Foreground screen | Optional Dim/Screen off controls, Never by default; text entry stays awake | SDK default tick suppresses OS idle timers while app timers manage brightness. Home remains owned by shell; device acceptance pending |
| Home playback profile | Separate ATTRIBUTE=32776 native package; graphics/input gating retained | User confirms preview.3 plays on Home. Preview.4 device acceptance pending. Game-triggered forced close remains unsafe |

The observed Spotify playlist menu contained Custom order, Title, Artist, Album,
Recently added, Release date and Duration, plus Compact/List presentation. Its
library menu contained Recents, Recently added, Alphabetical, Creator and Custom
order, plus compact/default list/grid choices. These are distinct sets of controls.

## Highest-available source quality

The selector now resolves track metadata once, then tries advertised sources in
this order: 24-bit FLAC, FLAC, Vorbis 320, MP3 320, MP3 256, Vorbis 160, MP3 160,
Vorbis 96, MP3 96. Missing formats cause no download attempt. A candidate that
cannot open/decode or exceeds its bounded startup attempt falls through to the
next source. The successful format is recorded by the player; the UI says
**lossless first**, not that an unobserved stream is lossless.

Best available is the default. Explicit Medium (160 kbps) and Low (96 kbps)
choices constrain the format set and never fall back upward; they persist and
apply to newly loaded tracks. The API also accepts a 320 kbps lossy cap. FLAC support was already present in the pinned Symphonia
build; the old selector never chose it. No decoder dependency upgrade was needed.

Spotify/account entitlement and actual advertised files determine whether FLAC
is available. Vita's existing sink outputs signed 16-bit PCM at 44.1 kHz and
normalisation/volume processing remains enabled. Preferring a 24-bit FLAC source
is **not** a claim of bit-perfect 24-bit output. Actual FLAC delivery, CPU/buffering,
seek and a long playback soak remain physical-device checks.

Primary references: [Spotify audio quality](https://support.spotify.com/uk/article/audio-quality/)
and [lossless requirements](https://support.spotify.com/us/article/lossless-audio-quality/).
The source selection and decoder support were checked in the pinned local code.

## Like synchronisation

The song overlay, player bar, playlist rows, context sheet and all retained pages
share saved-state updates. Pending changes survive repeated player snapshots and
new page replies. A failed write restores the prior state; success is shown only
after Spotify acknowledgement. New track metadata and Connect snapshots obtain
hearts from the authoritative saved cache rather than defaulting to false.

Library fetches and writes share a serialisation gate, so a slower read cannot
replace a later write. An account epoch guards synchronous commits and discards
old-session results after logout/account change. The backend checks for external
library changes at a 15-second interval, retains the last good state after a
failed fetch and invalidates affected views for refresh. This is eventual account
sync, not an instantaneous push guarantee. Loaded Liked Songs membership refreshes
on successful changes; a like does not modify an ordinary playlist's membership.

Offline regression checks cover playlist/player/Now Playing hearts, pending
state under stale snapshots, failure rollback, simulated external like/unlike,
filter/history retention and acknowledged playlist creation. A live two-way test
against Spotify desktop and the Vita is still pending; mocks do not establish
that a Spotify server accepted an edit.

## Remaining work

- Cloud pins and exact desktop custom-order persistence need authoritative
  service metadata. Local pins/collapse are implemented, bounded to 64 shared
  per-account/URI preference records; they do not modify cloud pins or folders.
- Playlist and queue mutations now have revision guards and duplicate-safe
  adapters. Live ownership/entitlement, queue continuity and host reconciliation
  still require a Spotify account and physical Vita acceptance.
- Artist biography/credits and richer discovery require supported data sources.
  Existing recently played shelves, album/artist navigation and lyrics remain.
- Download/offline mode, video, Smart Shuffle, Mix, social activity, collaborators,
  tour content and a device picker remain deferred backend/service projects.
- Preview.7 Home is generally accepted as release 0.1.0 by user report. Specific display/UI behavior,
  FLAC availability, host-device like reconciliation, API edit
  entitlement, Wi-Fi recovery, long soak, Home/return and orderly exit need device
  evidence. Separate service/IPC integration, game coexistence and sleep remain
  outside this UI milestone. Do not repeat game-triggered forced closure during
  playback; that transition previously hung the shell.

Playlist editing uses documented 2026 endpoints, without changing client IDs or
asking for credentials: [create private playlist](https://developer.spotify.com/documentation/web-api/reference/create-playlist),
[change details](https://developer.spotify.com/documentation/web-api/reference/change-playlist-details)
and [append items](https://developer.spotify.com/documentation/web-api/reference/add-items-to-playlist).
Scope denial is a supported error outcome, not something the client bypasses.
Read-only rootlist markers are also described by the upstream
[go-librespot API](https://pkg.go.dev/github.com/devgianlu/go-librespot@v0.10.5/spclient#Spclient.Rootlist).

Positional playlist edits use the pinned playlist4 protobuf schema: a revision,
exact range and entry identity are checked before a single non-replayed POST.
The reference protocol behavior was compared with
[go-librespot v0.10.5](https://github.com/devgianlu/go-librespot/blob/v0.10.5/spclient/library_write.go).
Timeout or ambiguous acknowledgement requires refresh before retry; the app never
replaces a partially loaded playlist. Queue edits use the existing owning Spirc
task, preserve context entries and reject remote/inactive playback.

## Evidence boundaries

Windows CTest, offline frontend exercises, real Rust unit tests and the native
cross-build are recorded separately. Native packages retain frontend ELF/VELF and
backend debug information. Source preparation verifies hashes; the native builder
validates metadata, CRCs, artwork, imports and output hashes. None installs a VPK,
reads account files, performs a live Spotify mutation, or proves hardware playback.

Preview.5 replaces Vita system-dialog text input with an in-app touch/controller
keyboard; see [release notes](releases/preview5.md) for the suspected freeze cause
and native acceptance limits.

Preview.7 implements the loading and playback-limit recommendations; see the
[implementation report and device acceptance](releases/preview7.md). Cached browse
data is labelled and cannot authorize edits or overwrite authoritative saved sets.
