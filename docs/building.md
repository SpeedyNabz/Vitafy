# Build and test

Run commands from the repository root. `python` may be `py` on Windows.
No command requires a Spotify account for mock testing. Current source builds
0.1.1-preview.1 (Home 01.31 / foreground 01.32, ABI v6). Published 0.1.0 uses the
unchanged preview.7 Home 01.29 binary; use tag `v0.1.0` for that source.
Current findings and remaining acceptance are in the [open work plan](open-work-plan.md).

## Pinned baseline

| Input | Selected value |
| --- | --- |
| Upstream | VitaSpot `81c533f1ef61623360ea4fbe32cf29b4f2f3901f` |
| VitaSDK | `sdk-snapshot-20260825.611.1`, channel 2026.08, core 2026.08.1-1 |
| Target packages | `packages-2026.08-snapshot-20261024.61.1` |
| ARM C compiler | GCC 15.2.0 |
| Rust | nightly-2026-10-18, rust-src, build-std |
| Target | `armv7-sony-vita-newlibeabihf` |
| Windows SDL | SDL2 2.32.10 / image 2.8.12 / ttf 2.24.0 |
| Target SDL | SDL2 2.32.8 / image 2.8.12 / ttf 2.24.0 |

Exact versions and checksums are in [source-lock.json](../source-lock.json).
These are recorded working inputs, not claims about latest available releases.

## Prepare source

```sh
python scripts/prepare.py
```

The script fetches the pinned upstream into `.cache/upstream/`, exports its commit,
applies `patches/0001-vitafy.patch`, copies the recorded overlays and verifies
hashes in `build/source/`. It never modifies the upstream checkout. An optional
`--upstream /path/to/exact/checkout` uses a local checkout's committed tree; its
HEAD must match the pin and uncommitted edits are not exported.

Generated sources are verified on reuse. If deliberately changing an overlay or
patch, review and update the source lock, then prepare a new clean generated tree.
Do not edit generated sources as the authoritative implementation. A failed
preparation is left for inspection rather than silently deleting files.

## Windows desktop

Prerequisites: Visual Studio 2022 Build Tools with the C++ workload and Windows
SDK, CMake 3.24+, Git, Python 3.9+.

```powershell
python -m pip install -r requirements-dev.txt
python scripts/build.py desktop
```

SDL archives are downloaded from the locked official release URLs and verified
before extraction. CMake builds the real frontend against the mock backend.
CTest runs display-order/snapshot, Premium, delayed login, free-account rejection and notification tests.
The UI exercise verifies search/library/Liked Songs/Now Playing/queue navigation,
simulated overlay drawing suspension, foreground return and quit behavior. A second
exercise covers likes, rollback, external changes, scoped filters, history, private
playlist creation and grid layouts using the offline mock.

To explore without audio:

```powershell
$env:VAM_MOCK_INSTANT = '1'
.\build\desktop\Release\VitafyPreview.exe
```

Arrow keys navigate; Enter confirms; Space toggles playback; Q/E skip;
N opens Now Playing; M opens queue. Mouse and controller input are also supported.
Default desktop account data is under the SDL per-user Vitafy preference directory.

## Linux desktop

Install your distribution's compiler, CMake, pkg-config, SDL2, SDL2_image and
SDL2_ttf development packages. Then run the same `python scripts/build.py desktop`.
The executable is `build/desktop/VitafyPreview`. Windows is locally validated;
the GitHub host workflow records Linux results separately.

## Native Vita build

Install the SDK and libraries above, CMake/Ninja, and the pinned Rust nightly
with rust-src. The SDK runtime, SDL/image/font/audio stubs and Rust C compiler
must all be available. The old Windows libpng alias repair is described in the
[toolchain research](research/toolchain-setup.md); adapt paths to your installation.

```sh
rustup toolchain install nightly-2026-10-18 --component rust-src
python scripts/build.py vita --sdk /path/to/vitasdk
```

`VITASDK` may replace `--sdk`. The script builds the real Rust backend with
`--locked` and `-Z build-std`, compiles the newlib/POSIX shim, finds ring's assembly
archive, and cross-compiles the frontend as RelWithDebInfo. It validates SFO,
SELF/CRC, package art and native audio/notification imports before copying the VPK
and hashes into `artifacts/Vitafy-0.1.1-preview.1/`. ELF/VELF remain in `build/native/`.
The fresh backend now preserves release debug information as well. Cargo's active
build-script record selects the matching ring assembly archive when multiple
profile variants exist. Source reconstruction, native linking and packaging have
been executed; actual audio and shell stability remain device checks.

ABI v6 adds cancellable chunked ordered uploads, stale-page flags, browse-cache
controls and resource metrics to entry identity/page ownership, playlist
and revision-aware entry/queue edit exports. The historical `--reuse-backend` ABI v3 archive is rejected for this
source; use the normal fresh Cargo build. No Spotify login is required to build.

## Experimental Home playback profile

```sh
python scripts/build.py vita --sdk /path/to/vitasdk --home-playback
```

This uses a separate `build/native-home/` and
`artifacts/Vitafy-0.1.1-preview.1-home-experimental/`, install version 01.31 and
ATTRIBUTE=32776. Debug ELF/VELF are retained. The default foreground build keeps
01.32 / ATTRIBUTE=32768. The Home profile reuses the historical no-suspend flag
and existing graphics lifecycle gate; device acceptance is pending. Do not accept
a game-triggered forced-close prompt during playback: that transition previously
hung the shell. Pause and use Settings → Exit before opening another app.
Foreground 01.32 can replace Home 01.31. Switching back to Home after installing
01.32 requires a newly versioned Home build; choose your profile before installation.

Build tools may emit the installed SDK's existing `crtn.o` GNU-stack warning.
No warning-free or memory-safety audit is claimed. Original lab inputs and the
standalone repack have separate artifact hashes because target names, shim debug
information and build paths differ.

## Real backend host checks

```sh
cargo +nightly-2026-10-18 test --offline --lib --manifest-path build/source/backend/librespot-vam/Cargo.toml
```

This runs real Rust logic tests without credentials or Spotify network requests.
It is separate from the C/frontend mock checks. Dependencies must already be
cached for `--offline`. The current suite has 32 tests, including source-quality
priority, saved-cache consistency, account epochs, metadata preservation, duplicate-safe playlist edits and queue
context preservation, persistent cache recovery/budgets and real large-context
serialization/rolling windows. Five frontend UI suites include 5,000-song loading
and explicit order fallback. Nine CTest cases cover host contracts and preferences.
The 32 Rust tests also cover app software gain/mute, early-preload headroom,
obsolete-loader cancellation/deadline and stale playback-event rejection. See
[candidate host evidence](evidence/bluetooth-skip-host.json). Native symbol checks
confirmed generic system-volume getter/setter calls are absent and debug sections
remain; [native evidence](evidence/bluetooth-skip-home-native.json) does not imply
Bluetooth or audible skip-latency acceptance.

When deliberately editing canonical overlays, review changes, then run
`python scripts/refresh_sources.py --accept-overlay-edits` to update their lock
hashes and refresh the generated tree. Ordinary builds only verify hashes; they
do not silently accept edits. Never use the generated source as the canonical copy.
