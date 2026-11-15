# Roadmap

The [desktop UI audit and implementation plan](desktop-ui-audit.md) records the
15 November live Spotify comparison and preview.3 implementation: lossless-first
source selection, synchronised hearts, full metadata sorts, scoped filters,
presentation/history, folders and acknowledged playlist creation/rename/append.
Preview.3 adds guarded playlist/queue edits and device-local pins/collapsed folders.
Cloud pin sync and live service/device acceptance remain pending. The Home build remains experimental.

Preview.4 adds an always-on foreground display, stacked library filters and sorting
dropdowns. Playlist pages automatically load all songs and use plain list rows.
The user confirms preview.3 playback on Home; new display/UI acceptance remains
pending. Downloaded filtering is present but empty until an offline store exists.

1. Validate the new frontend's actual audio, Unicode shell notifications and
   controlled shutdown on a Vita; collect minimal numeric evidence.
2. Verify authentication/relaunch, Wi-Fi interruptions, reconnect, queue/seek,
   long playback and bounded memory/cache behavior.
3. Finish the separate service's bounded real-audio test and active-audio teardown
   before integrating it with the frontend.
4. Build a bounded versioned IPC adapter with reconnect/resynchronization and
   explicit backend event ownership.
5. Prove active service lifetime and resource budgets before any controlled game
   coexistence test. Do not repeat the known unsafe game-close transition.
6. Investigate display-off, power button and true suspend as separate states.
7. Clarify upstream frontend/assets redistribution terms and complete dependency
   notices before a public binary release.

These are pending acceptance milestones. Source publication, successful compiling,
mock checks and idle-service observations do not close them.

Preview.7 implements the [loading/cache plan](releases/preview7.md). Next: measure
cold/warm browsing and IO/RAM on hardware; verify 10,000-entry sorted playback,
shuffle/repeat, handoff and audio soak. Host coverage and native builds pass,
while these physical acceptance tasks remain pending.

Release 0.1.0 promotes preview.7 Home unchanged after user-reported device
acceptance. See [release notes](releases/0.1.0.md); remaining stress/lifecycle
checks above retain their narrower evidence requirements.
