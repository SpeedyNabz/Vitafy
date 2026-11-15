# Preview.4 — screen and library controls

Preview.3 Home playback continues on the Home screen by user report. Preview.4
keeps that Home configuration and updates the foreground screen and library UI.

## Changes

- The foreground display stays on while browsing, paused or playing. Legacy
  dim/off preferences and the app screen-off combo no longer darken the display.
  The pinned SDK's `sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT)` cancels
  dim/off/suspend timers only while Vitafy owns graphics. Home retains the shell's
  display policy; physical power/suspend controls are not disabled.
- Library category chips (Playlists, Albums, Artists) combine with independent
  By you and Downloaded modifiers. Tapping an active chip removes it. The sidebar
  follows the same category/modifier selections. One category is selected at a
  time, as in Spotify's library; ownership and download status are intersections.
- Downloaded honestly shows no matches. Vitafy has no offline-download store;
  cached streaming audio is not presented as downloaded music.
- Playlist and Liked Songs pages automatically fetch all available pages without
  scrolling. The old 2048-song display cutoff is removed. Filter, Compact/List,
  Refresh and Load all controls are removed from those pages. They always show
  list rows; legacy view filters cannot hide songs. A failed load shows an
  explicit retry action, and a partial playlist is never labelled complete.
- Sorting opens a dropdown in playlist pages, Your Library and the sidebar.
  The active option and direction are highlighted. Choose another option to
  apply it; choose the active option again to reverse direction. Outside click
  and Back/Escape dismiss the menu without navigating or activating content.

Existing lossless-first source selection, shared hearts, safe playlist/queue
edits, local pins/folders, navigation, lyrics and notifications are retained.
Library text search and presentation options remain available.

## Bounds

Loading remains asynchronous in 50-entry frontend slices and validates revision,
page offsets and allocation success. Backend playlist pagination no longer has
the old 5000-entry cutoff. Memory or network failures remain visible errors;
the app does not promise unlimited device memory. Explicit sorted playback still
accepts at most 2048 songs; displaying/loading larger playlists is supported.
The independent root library fetch remains bounded to 5000 entries.

## Package and validation

The Home VPK is install version **01.23**, ATTRIBUTE=32776, title ID `VTAM00001`.
It can update preview.3 Home 01.21 or foreground 01.22 and retains
`ux0:data/VitaSpot/`. Manually exit the current player before updating; keep a
private data backup. No installation was performed in this work.

Source is pinned to VitaSpot `81c533f1ef61623360ea4fbe32cf29b4f2f3901f`,
VitaSDK `sdk-snapshot-20260825.611.1`, GCC 15.2.0 and Rust nightly-2026-10-18.
No dependency upgrade or backend ABI change (v5). The power-tick declaration and
default timer type were checked in the installed SDK's processmgr headers and
the existing SceProcessmgr link stub; native packaging verifies the import.

Executed: eight Windows CTest cases, three offline frontend suites and 16 real
Rust tests; native Home cross-build and package/import checks. The new regressions
cover a complete 320-song load without scrolling, stacked filter behavior,
dropdown selection/direction/dismissal and legacy power deadlines/combo. Visual
review checked chip spacing, menu placement and simplified playlist presentation.
Frontend ELF/VELF and Rust backend debug information are retained locally.
Existing SDK/dependency warnings remain.

Evidence: [host](../evidence/preview4-host.json),
[native Home](../evidence/preview4-home-native.json),
[preview.3 user report](../evidence/preview3-home-user-report.json).
Offline mocks establish UI behavior, not Spotify service acceptance.

New display behavior while idle/paused and the updated UI still need physical
Vita acceptance. Live FLAC delivery, host/Vita like reconciliation and editing
acceptance remain separate checks. **Do not accept game-triggered forced closure
while playing:** the prior shell hang is not fixed here. Exit before launching
a game. True sleep and service/game coexistence remain deferred.
