# Current status

This is the authoritative status page for Vitafy 0.1.0-preview.1. Historical
research files retain the conclusions and pending statements from their own dates.

| Evidence | Result |
| --- | --- |
| Original VitaSpot local Spotify playback | Working by user report on a modded PCH-1003 |
| Standalone source reconstruction | Exact upstream pin, reviewed patch and overlay hashes verified locally |
| Standalone Windows frontend and mock backend | Builds; all four CTest scenarios pass |
| Standalone UI exercise | Search/library/Liked Songs/Now Playing/queue/settings and overlay/quit checks pass |
| Native Vitafy frontend/package | Cross-build and imports/package checks recorded separately |
| Vitafy actual audio / notification delivery | Device pending |
| Fresh Rust rebuild through standalone wrapper | Pending; native check reused hash-pinned real backend |
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

Vitafy uses ATTRIBUTE=32768 rather than enabling the unsafe experimental residency
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
