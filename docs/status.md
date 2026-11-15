# Current status

This is the authoritative status page for Vitafy 0.1.0 (preview.7 build). Historical
research files retain the conclusions and pending statements from their own dates.

## Release 0.1.0

On 15 November 2026 the user confirmed that preview.7 works as intended and
approved it as a release. Version 0.1.0 promotes the identical Home 01.29 binary,
without changing its tested contents or source-lock build identifier. See the
[release notes](releases/0.1.0.md) and [user device report](evidence/preview7-home-user-report.json).
General user acceptance does not establish unmeasured stress/performance results;
the known game forced-close issue and specific pending checks remain documented.

## Preview.7 local browsing and larger sorted playback

Implemented the loading investigation: single-flight playlist snapshots, account-scoped
persistent metadata/library summaries with atomic recovery, bounded LRU eviction,
known-unknown field freshness, separated root/saved refresh intervals, cache settings
and diagnostics. Ordered playback counts playable entries, supports up to 10,000
through cancellable chunk uploads, preserves duplicate positions and offers an
explicit original-order fallback. Existing Connect 80-next/10-previous rolling
windows were validated with the real pinned state code.

28 Rust tests, nine CTest cases, five offline UI suites and a fresh native Home
01.29 / ABI v6 build passed. No new Vita installation, live Spotify mutation,
performance benchmark or audible continuity test was performed. See
[preview.7](releases/preview7.md) for exact budgets, evidence and device acceptance.
The following preview sections are historical.

## Preview.6 library and full-screen keyboard

Removed the redundant local playing label; buffering, connection, remote-device
and paused messages remain. Your Library now always uses list rows, with category/
By you chips, a count and the right-aligned sort dropdown. Text filtering, grid,
Compact/List, Refresh and manual Load all controls are removed. Retained old view
preferences cannot hide rows or restore those layouts. Library pages load forward
automatically, with retry only after an error.

The keyboard fills 960x544 and uses 88x58 character targets, larger modifier keys,
a wider text field and large Done/Cancel buttons. Host regression tests exercise
the new touch positions and controller cancellation. Native Home packaging and
actual device acceptance are separate evidence.

The [loading/cache/sorted playback investigation](loading-cache-and-sorted-playback-investigation.md)
records current caches, repeated full-playlist fetches, RAM cache eviction,
account-safe persistent metadata proposals and options for the app's explicit
2048-song ordered-playback limit. These proposed optimizations are not implemented
in preview.6; the limit remains. Eight CTest cases and four offline UI suites pass;
the wider-key positioning issue found by the keyboard test was corrected and that
suite rerun. No new physical Vita test or cache benchmark was performed.

## Preview.5 timers, quality and search

Screen dim/off controls are restored, both defaulting to Never. Saved supported
choices persist. Input wakes dimmed displays; a button wakes an off display and
is consumed. App screen-off shortcuts remain disabled. Home's display policy
and the successful Home playback configuration are retained.

Best available remains the default lossless-first quality. Medium caps source
selection at 160 kbps and Low at 96 kbps; changes apply to newly loaded tracks,
including invalidating preloads made at the previous quality. Downloaded has
been removed from the library filters.

Search, library filtering and playlist naming use an in-app touch/controller
keyboard on Vita. This avoids opening a system keyboard overlay while lifecycle
rendering is gated. The suspected overlay/rendering conflict has not been
confirmed by a new device trace, and the replacement still needs device testing.
The keyboard offers Latin letters, numbers and punctuation; existing UTF-8 text
is preserved and deletion respects UTF-8 character boundaries.

Eight CTest cases, four offline UI suites and 17 real Rust tests pass. Native
Home packaging is recorded separately in [preview.5 notes](releases/preview5.md)
and [native evidence](evidence/preview5-home-native.json). No new installation,
physical audio measurement, live Spotify write or device freeze test was run.

## Preview.4 screen and library update

The user confirms preview.3 Home playback continues on the Home screen. This is
user-reported physical evidence for that build; no new agent device test was run.
See [the report](evidence/preview3-home-user-report.json).

Preview.4 keeps the display on while Vitafy is foreground, even paused. Legacy
idle timers and the screen-off combo no longer darken the app; the shell retains
its own display policy on Home. Library category chips combine with By you and
Downloaded and apply to the sidebar. Downloaded is empty because no offline
store exists. Sorting now uses dropdowns, including the sidebar; selecting the
active sort toggles direction. Playlist pages use automatic complete loading and
plain list presentation, without Filter, Compact/List, Refresh or Load all controls.
Source revision/offset and allocation failures remain explicit errors.

Eight CTest cases, three offline UI suites and 16 real Rust tests pass. The new
320-song regression verifies loading to completion without scrolling. See
[preview.4 release notes](releases/preview4.md) and its separate host/native evidence.
New screen power behavior and UI acceptance on the Vita remain pending. The
known game forced-close hang is not fixed.

## 15 November preview.3 consolidated build

Preview.3 includes the preview.2 desktop audit features and adds revision-guarded
single-entry playlist removal/reordering, explicit queue removal/reordering/clear,
Vita-local pins and collapsible sidebar folders. Playlist removal is confirmed;
queue clear preserves the playing song and context. Headers with descriptions
and the sidebar Forward control have corrected spacing.

Seven CTest cases, the two existing offline UI exercises and 16 real Rust tests
pass. The final native Home and foreground packages are checked separately;
see [release notes](releases/preview3.md) and [host evidence](evidence/preview3-host.json).
Both use fresh ABI v5 backend builds and retain debug information. Home is 01.21,
foreground is 01.22 so it can replace Home. Neither was installed in this work.
Live edits, actual highest-quality playback, two-way host heart sync and physical
Home/return/lifecycle acceptance remain pending. The known game forced-close hang
is not fixed by this UI build.

## 15 November preview.2 implementation

The [desktop audit implementation status](desktop-ui-audit.md) is the feature and
limitation ledger. Preview.2 adds all seven playlist sorts with real metadata,
revision-checked bounded snapshots, sorted playback, local filters, grid/compact
views, back/forward, read-only folders and acknowledged create/rename/append.
Quality is lossless first with descending source fallback; the Vita sink remains
16-bit PCM. Hearts share state across views, pending writes survive snapshots,
failures roll back and external account changes reconcile periodically.

Executed checks: five CTest scenarios, two offline frontend exercises and 12 real
Rust tests pass. Live Spotify edits, FLAC delivery, actual two-way desktop/Vita
like sync and physical lifecycle acceptance remain pending. The native package
uses a freshly rebuilt ABI v4 backend with debug information, rather than the
historical ABI v3 archive. Home install version is 01.17 (experimental); the
foreground profile is 01.16. Neither profile was installed during this work.

Evidence: [preview.2 host checks](evidence/preview2-host.json) and
[fresh backend / native Home package](evidence/preview2-home-native.json).
The ELF's `.debug_info` and `.debug_line` sections were inspected. Native linking
still emits the installed SDK's existing GNU-stack warning; upstream Rust
dependency warnings are retained in local logs. No warning-free claim is made.

## Earlier 15 November preview.1 UI work (historical baseline)

Implemented a persistent scrollable playlist sidebar, alphabetical library view,
and playlist Custom order/Title/Artist/Duration display sorting with direction.
Original context offsets survive sorting. Sort applies to loaded songs and does
not change Spotify's subsequent playback order. Album/Recently added/Release date
sorts and full library sort parity require a metadata/backend follow-up; see the
[live desktop audit and prioritised plan](desktop-ui-audit.md).

The updated Windows frontend builds, all five CTest cases pass and the extended
UI exercise passes, including sidebar playlist navigation, title sorting and
graphics overlay/return/quit behavior. [Host evidence](evidence/library-ui-host.json)
is mock-only: it produces no Spotify audio.

An opt-in `--home-playback` profile uses ATTRIBUTE=32776, install version 01.15,
and separate output directories. The normal default remains foreground. This is
an experimental package configuration, **not a fixed game-close lifecycle**.
No new hardware result, installation or service integration is claimed.

The final Home profile cross-build passed package/CRC/artwork/import checks using
the hash-pinned existing real backend archive; ELF/VELF are retained locally.
[Native evidence](evidence/library-ui-home-native.json) records SDK/source/backend
and output hashes. The toolchain still emits its existing `crtn.o` GNU-stack
warning. A fresh Rust rebuild and actual Home audio/return/exit remain pending.

| Evidence | Result |
| --- | --- |
| Original VitaSpot local Spotify playback | Working by user report on a modded PCH-1003 |
| Standalone source reconstruction | Exact upstream pin, reviewed patch and overlay hashes verified locally |
| Current Windows frontend and mock backend | Builds; all five CTest scenarios pass |
| Standalone UI exercise | Search/library/Liked Songs/Now Playing/queue/settings and overlay/quit checks pass |
| Native Vitafy frontend/package | Cross-build and imports/package checks recorded separately |
| Vitafy actual audio / notification delivery | Device pending |
| Preview.2 real Rust backend | Fresh cross-build; 12 host unit tests pass; live service acceptance pending |
| Linux host build | GitHub workflow result is separate from Windows local evidence |
| Game coexistence, forced close, Wi-Fi recovery, long soak, power/sleep | Pending |

The [historical lab preview record](evidence/lab-preview.json) identifies the
earlier lab build, not the standalone repack. Local builds generate their own
verification records with source locks and artifact hashes. No mock executable or
VPK is labelled working Spotify playback.

The publication preparation also retains path-free [standalone Windows results](evidence/standalone-host.json)
and [standalone native package results](evidence/standalone-native.json). The native
result uses the verified existing playback archive and records that limitation.

## Known lifecycle failure

Home.1 played from Home, including manual closure/reopening, by user report. A
game's forced-close prompt during playback hung the shell and required reset.
Removing BGM ownership in Home.2 lost Home audio and was rejected as a background
fix. This did not establish the hang's root cause.

The default Vitafy profile uses ATTRIBUTE=32768 rather than enabling experimental residency
flags. That is a foreground package choice, not proof that forced-close teardown
is fixed. **Do not test game-triggered forced closure while playing.**

## Separate service research

The paired background service passed launch/lifetime, bounded allocations,
worker-only LiveArea persistence and real backend logged-out startup/shutdown,
with the evidence limits recorded in the historical ledger. The bounded 0.6
service audio test had no device result at publication preparation. The current
Vitafy frontend does not implement the IPC transport to that service.

All reported device observations used a modded PS Vita PCH-1003. Exact firmware
and plugins were not recorded; compatibility with other devices is not established.
Publication is a source/documentation milestone, not a new hardware test result.
