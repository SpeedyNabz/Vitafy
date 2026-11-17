# Bluetooth volume and long Next buffering

Development build: **0.1.1-preview.1**, Home **01.31**, backend ABI v6.
Prepared 17 November 2026. The released 0.1.0 binary is unchanged.

## Report and evidence boundary

The user reports that headphone playback volume jumps between the Vita device
level and its Bluetooth-specific level, and that Next sometimes buffers for a
long time. These observations are user-reported; no Bluetooth device or real
Spotify audio was accessed during this change. The code below explains plausible
causes and implements targeted fixes. It does not establish a device-measured
root cause or promise instant skips on a slow connection.

Pins remain VitaSpot `81c533f1ef61623360ea4fbe32cf29b4f2f3901f`, VitaSDK
`sdk-snapshot-20260825.611.1`, GCC 15.2.0 and Rust nightly-2026-10-18. The installed
`psp2/avconfig.h` declares generic GetSystemVol/SetSystemVol and a 0-30 setter
range. It provides no documented Bluetooth-volume selector. Existing stubs and
source are used; no new Vita API or dependency was introduced.

## Volume changes

The previous engine polled system volume every 50 ms, forwarded detected changes
through Connect, and immediately applied slider changes through both the mixer
and the asynchronous Connect command queue. The mixer wrote generic AVConfig
volume and only suppressed matching echoes for two seconds. A rejected write
permanently switched that mixer to software gain. With route-specific hardware
levels, this creates competing authorities rather than identifying which output
level a Bluetooth read represents.

The new build always uses the existing software mixer for **app/Spotify volume**.
It neither reads nor writes system volume. The unused AVConfig volume declarations
and platform helpers are removed from the canonical source. Connect is the sole
writer after initialization; coalesced slider commands are no longer also applied
directly. Zero volume is now accepted by snapshot reconciliation instead of being
ignored. The expanded player labels the slider **App volume**.

Expected behavior: the Vita/headset buttons control their route's hardware volume;
the in-app slider and Spotify Connect control app gain independently. The app
slider does not mirror physical volume buttons. The app starts at unity gain on
Vita, leaving the user's existing hardware level in control. Normalisation and
all Best/Medium/Low quality policies remain as before.

## Next-track changes

The previous player only suggested a preload within the final 30 seconds of a
song, and only after the entire remainder was downloaded. An earlier Next tap,
particularly with lossless files, commonly had no prepared next song. Changing
songs dropped the receiving future, but the separate loader thread could continue
its asynchronous work, competing with the newly requested track.

The new player:

- Suggests one next-track preload after two seconds of playback when approximately
  three seconds of contiguous encoded audio are buffered ahead, or the current
  file is complete. The normal read-ahead target is five seconds. The prior
  short-track/end-of-song complete-file condition remains available. Buffer
  duration is estimated from nominal bytes per second, not decoded PCM duration.
- Reuses the existing single Loading/Ready preload slot, decoder and disk audio
  cache. It does not fetch an unbounded queue or lower the selected source quality.
- Refreshes the next target after queue/shuffle/context/autoplay changes. A target
  comparison avoids duplicate commands; active-device state gates preparation.
- Keeps failed speculative preloads out of the Unavailable event path. A network
  failure during prefetch no longer marks that queued song unavailable or makes
  the frontend report that current playback failed. A real foreground load can
  retry and retains the existing error/advance handling.
- Cancels asynchronous loading when its result consumer is dropped. Async phases
  have a 45-second deadline. Blocking decoder, file and transport reads still use
  their underlying IO behavior; this deadline cannot interrupt arbitrary blocking
  calls or guarantee a 45-second wall-clock cap on every stall.
- Filters progress/error events by current play-request ID, so delayed old events
  cannot restore stale buffering or playback after another skip. Session teardown
  resets that identity.

Starting the preload needs enough headroom; an immediate skip after selecting a
song may still be a cold load. Slow Wi-Fi, service/key latency, decoder work and
Bluetooth/Wi-Fi radio contention remain possible sources of delay. No new route
switching, radio scheduling or forced audio-port reinitialization is attempted.

## Validation

32 real Rust tests pass, including route-independent soft gain/mute, early-preload
headroom and short/cached-track cases, dropped/timed-out async loader futures,
and stale playback-event rejection. The soft mixer's mapped-volume roundtrip can
differ by one u16 unit, so the regression permits that rounding. Nine desktop
CTest cases and five offline UI suites pass. Native Home build/package and symbol
checks are recorded separately in the evidence files below.

[Host evidence](evidence/bluetooth-skip-host.json),
[native Home evidence](evidence/bluetooth-skip-home-native.json).
No test here measures real Bluetooth volume, skip latency or audible continuity.

## Device acceptance

At a comfortable hardware level, verify speaker, wired and Bluetooth playback;
connect/disconnect the headset while playing; use Vita/headset volume buttons;
then use the app slider and remote Spotify slider, including mute/unmute. App
volume should stay independent and physical level changes should not be written
back to another route.

On the same playlist/network/quality, compare Next immediately after track start,
after 5-10 seconds, and near the end. Repeat with rapid Next taps, unavailable
songs, shuffle, explicit queue changes, sorted playback, Home/return and Wi-Fi
interruption. Note actual latency separately from the buffering indicator. A
failed speculative fetch must retain the queued song and not show a false current
track error. Check sustained playback for underruns and bounded resources.

Use Settings > Exit before launching a game; the existing forced-close issue is
unchanged. This is a test build pending user acceptance, not a replacement release.

## Open work tracking

No new device acceptance arrived with this documentation update. Follow A1-A2
in the [open work plan](open-work-plan.md) for candidate acceptance and remaining
blocking-IO diagnostics. Worker/game requirements are separate B0-B6 gates; this
volume/preload patch does not make the current process survive game-forced closure.
