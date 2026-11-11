# Vitafy working agreements

- Read README.md, docs/status.md and only the research relevant to the task.
- Preserve the pinned upstream and source hashes; never edit generated sources as the canonical implementation.
- Keep portable logic separate from Vita platform code and validate SDK declarations/stubs.
- The target is native local playback, not a remote controller.
- Preserve backend event ownership, bounded work and nonblocking UI behavior.
- Keep debug evidence; distinguish mock, cross-build and device observations.
- Do not commit generated builds, credentials, private account data or raw device logs.
- Do not repeat known game-triggered forced-close hangs.
- Do not label pending hardware, service audio, game coexistence or sleep as verified.
- Preserve attribution and licensing findings; do not assign a blanket upstream license.
