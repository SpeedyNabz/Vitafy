# Preview.6 — simpler library and full-screen keyboard

Home package: **01.27**, ATTRIBUTE=32776, title ID `VTAM00001`. Retains
`ux0:data/VitaSpot/` and updates previous Home/foreground packages. Exit the
running player before updating. No installation was performed during this work.

- Removed “Playing on this Vita”. Paused, buffering, disconnected and remote
  playback status remain available.
- Your Library uses plain list rows with a count and a right-aligned sort
  dropdown, matching playlist controls. Category/By you chips and New playlist
  remain. Removed text Filter, grid, Compact/List, Refresh and manual Load all;
  old presentation preferences are cleared for library/playlist views. Library
  pagination proceeds automatically; errors retain a retry action.
- The in-app keyboard fills the whole 960×544 display, covering the sidebar and
  player bar. Character hit areas grow from 64×44 to 88×58, with larger Space,
  Delete, Shift, Done and Cancel targets. Touch/controller entry remains shared.

The [loading/cache/sorted-playback investigation](../loading-cache-and-sorted-playback-investigation.md)
documents actual code paths and possible follow-ups. It identifies repeated
full-playlist fetches for 50-row replies, metadata cache clearing at capacity,
existing disk caches and both explicit 2048-entry ordered-playback guards.
It recommends snapshot reuse, compact persistent metadata with account/revision
validation, measured cache budgets and an end-to-end large-context strategy.
These are investigation results, not implemented performance improvements or a
fix for the sorted-playback limit. No timings or physical RAM/IO profile was taken.

Pins remain VitaSpot `81c533f1ef61623360ea4fbe32cf29b4f2f3901f`, VitaSDK
`sdk-snapshot-20260825.611.1`, GCC 15.2.0, Rust nightly-2026-10-18 and ABI v5.
No dependency upgrade. Audio quality choices, Never-default screen timers,
shared likes and existing Home playback configuration are retained.

Validation: eight Windows CTest cases, four offline UI suites and native Home
build/package checks. The full-screen keyboard regression caught unsigned layout
arithmetic; this was corrected and the keyboard/settings suite passed again.
Visual review checked library controls and the full keyboard. Rust playback logic
is unchanged from preview.5; no new Rust unit-test run is claimed. Debug ELF/VELF
and backend information remain local. Existing SDK/dependency warnings remain.

Evidence: [host](../evidence/preview6-host.json),
[native Home](../evidence/preview6-home-native.json). New UI touch accuracy and
Home/return acceptance still need a physical Vita check. Preview.3 Home playback
remains confirmed by user report. The known game-triggered forced-close shell
hang remains unresolved; exit Vitafy before launching a game.
