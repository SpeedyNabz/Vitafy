# Preview.5 — optional timers, bandwidth choices and search input

Home package: install version **01.25**, ATTRIBUTE=32776, title ID `VTAM00001`.
It updates previous Home/foreground profiles and retains `ux0:data/VitaSpot/`.
Manually exit the running player before updating. No installation was performed.

- Screen settings again offer Dim screen after and Turn screen off after. Both
  default to Never. Supported saved selections persist. Text entry stays awake;
  small stick drift does not reset idle timers. A button wakes an off display
  without activating the control underneath. The app's manual screen-off combo
  remains disabled; Home retains the shell's display policy.
- Audio quality cycles through Best available, Medium (160 kbps), Low (96 kbps).
  Best remains the default and prefers lossless with descending fallback.
  Medium/Low select only advertised formats at or below their cap and never
  fall back upward. A change applies to newly loaded songs, with old-quality
  preloads discarded. It does not interrupt the current song or transcode audio.
- The Downloaded filter chip is removed. Category and By you filters remain.
- Search, library text filtering and playlist names use an in-app keyboard on
  Vita. Touch or D-pad/Confirm selects characters; Done commits, Cancel/Back
  discards changes. Latin letters, Shift, digits and punctuation are provided.
  Existing UTF-8 text is retained and deletion respects character boundaries.
  Native IME language/composition features are not provided by this keyboard.

The search freeze's suspected cause is interaction between system-overlay text
entry and the existing rendering guard. The local app called SDL text input
while suppressing rendering for system overlays/intercepted input. SDL's
[Vita text input implementation](https://github.com/libsdl-org/SDL/blob/SDL2/src/video/vita/SDL_vitavideo.c)
uses the native keyboard/dialog path. This is a code-based inference, not a
physical-device trace. Preview.5 disables the SDL screen keyboard hint before
video initialization and uses the portable in-app keyboard without opening an
overlay. It preserves the existing Home rendering guard.

Pins remain VitaSpot `81c533f1ef61623360ea4fbe32cf29b4f2f3901f`, VitaSDK
`sdk-snapshot-20260825.611.1`, GCC 15.2.0 and Rust nightly-2026-10-18. Backend
ABI remains v5; dependencies were not upgraded. The installed SDK's power-tick
declaration and process-manager stub are retained. The source lock records all
canonical changes, including the explicit native keyboard source entry.

Validation: eight Windows CTest cases, four offline frontend suites and 17 real
Rust tests pass. New coverage includes default/persistent timer settings,
optional dim/off and consumed wake input, bandwidth caps, touch keyboard entry,
commit/cancel, controller input and settings quality selection. Visual checks
cover keyboard layout and settings. The native Home build verifies packaging,
metadata, CRCs, artwork, imports and hashes; debug ELF/VELF and backend symbols
remain local. Existing SDK/dependency warnings remain.

Evidence: [host](../evidence/preview5-host.json),
[native Home](../evidence/preview5-home-native.json). Real Vita search responsiveness,
idle timers, lower-bitrate source selection and Home/return still need acceptance.
Prior preview.3 Home playback remains confirmed by user report.

Existing limitations remain: PCM output is 16-bit/44.1 kHz with processing,
explicit sorted playback is bounded to 2048 songs, and offline downloads are
not implemented. The known game-triggered forced-close shell hang is not fixed;
exit Vitafy before launching a game. Sleep/game coexistence remain deferred.
