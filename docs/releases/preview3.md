# Preview.3 consolidated build — 15 November 2026

This build combines the implemented desktop UI audit, lossless-first source
selection and synchronised hearts with revision-aware playlist and queue edits.
It is a private development preview. No installation or live Spotify write was
performed while preparing it.

## Packages

Local output: `artifacts/Vitafy-0.1.0-preview.3-bundle/`. The bundle contains two
VPKs, their verification records, release notes and SHA-256 checksums.

| Profile | Install version | Purpose |
| --- | --- | --- |
| Home experimental | 01.21, ATTRIBUTE=32776 | Retains the historical Home residency configuration and graphics/input gate; new hardware acceptance pending |
| Foreground | 01.22, ATTRIBUTE=32768 | Normal foreground profile; higher version permits replacement of the Home experiment |

Both use title ID `VTAM00001` and retain `ux0:data/VitaSpot/`. Install only one
profile. Manually exit the running player and keep a private data backup before
updating through the normal homebrew installer. Foreground 01.22 can replace Home
01.21; returning to Home after that requires a newly versioned Home package.

**Do not launch a game while playing or accept a game-triggered forced-close
prompt. That transition previously hung the shell.** This build does not fix it.
Pause and use the in-app Exit action first. True sleep, game coexistence and the
separate service/IPC integration remain research milestones.

## Included features

- Persistent playlist sidebar with owner/current-playback indicators, local pins,
  folder navigation and collapsible descendants. Pins and collapse are per-account
  Vita preferences, bounded to 64 shared URI records; cloud pins are not synced.
- Seven playlist sorts with direction, loaded/all scope and cancellable Load all
  up to 2048 entries. Library sorts, By you, filtering, compact/list/grid views,
  presentation persistence and back/forward navigation are retained.
- Private playlist creation, owned-playlist rename, add-to-playlist, confirmed
  entry removal and move up/down. Mutations validate owner, current revision,
  original index, URI and available entry identity; duplicate songs stay distinct.
  Stale snapshots fail and require refresh. No whole-playlist replacement.
- Queue sections distinguish the playing song, explicit queue and context music.
  Remove, move and confirmed clear affect explicit queued entries only. Editing
  requires this active local player and a current revision; it never transfers
  remote playback silently.
- Lossless-first selection: advertised 24-bit FLAC, FLAC, then descending available
  lossy sources with bounded startup fallback. Old quality caps migrate. Actual
  account entitlement controls availability; the Vita sink remains 16-bit PCM at
  44.1 kHz with existing processing, so this is not bit-perfect 24-bit output.
- Shared hearts across player, song, sheet and playlist views; pending changes
  survive snapshots, failed writes roll back, acknowledged writes update the
  cache and external changes reconcile periodically. Server writes are bounded.
- Existing search, Home shelves, album/artist navigation, lyrics, seek, volume,
  shuffle, repeat, notifications, retry/status controls and orderly Exit.

Cloud pin/order writes, rich artist credits, offline downloads, video, social
features, Smart Shuffle and service/device integration remain deferred. See the
[audit ledger](../desktop-ui-audit.md) for limits.

## Reproducibility and evidence

Upstream VitaSpot is pinned at `81c533f1ef61623360ea4fbe32cf29b4f2f3901f`.
VitaSDK is `sdk-snapshot-20260825.611.1`, GCC 15.2.0, Rust
`nightly-2026-10-18`; dependencies were not upgraded. Backend ABI is v5. The
historical ABI v3 archive is rejected. Canonical sources and overlay hashes are
recorded in `source-lock.json`.

Playlist mutation protocol fields were verified against the pinned
`librespot-protocol 0.8.0` playlist4 declarations and
[go-librespot v0.10.5 protocol behavior](https://github.com/devgianlu/go-librespot/blob/v0.10.5/spclient/library_write.go).
The adapter builds its own protobuf ranges, sends once and reports ambiguous
results as refresh-before-retry errors.

Executed host checks: seven CTest cases, two existing offline UI suites and 16
real Rust tests. Visual menu checks cover scrolling, local pinning and description
spacing. Native builds verify SFO metadata, archive CRCs/artwork, required exports
and native audio/notification imports; debug ELF/VELF and backend symbols remain
locally available. Existing dependency and SDK GNU-stack warnings remain.

Evidence: [host](../evidence/preview3-host.json),
[Home native](../evidence/preview3-home-native.json),
[foreground native](../evidence/preview3-foreground-native.json).
These checks produce no Spotify audio and do not prove service acceptance.

Pending on a physical Vita: authentication/relaunch, audible output, negotiated
format and long soak, seek/queue continuity, live playlist editing and error
recovery, two-way hearts against Spotify desktop, network recovery, Home/return
and orderly Exit. Do not use the known unsafe game-close transition as a test.
