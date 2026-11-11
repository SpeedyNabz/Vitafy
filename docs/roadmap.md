# Roadmap

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
