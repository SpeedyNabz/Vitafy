# Build and test

Run commands from the repository root. `python` may be `py` on Windows.
No command requires a Spotify account for mock testing.

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
CTest runs Premium, delayed login, free-account rejection and notification tests.
The UI exercise verifies search/library/Liked Songs/Now Playing/queue navigation,
simulated overlay drawing suspension, foreground return and quit behavior.

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
and hashes into `artifacts/Vitafy-0.1.0-preview.1/`. ELF/VELF remain in `build/native/`.
The imported historical Rust archive itself has release debug stripping; retaining
frontend symbols does not restore missing backend debug information.

For reproducing the tested frontend/package using an already-built backend:

```sh
python scripts/build.py vita --sdk /path/to/vitasdk --reuse-backend --backend-dir /path/to/librespot-vam
```

This mode is read-only with respect to that backend directory and accepts only
the exact recorded historical archive hash. The directory must also contain its
ring assembly archive. The standalone native check used this mode; a fresh Rust
backend rebuild under the new wrapper is separately pending. No successful build
is a device playback or shell-stability result.

Build tools may emit the installed SDK's existing `crtn.o` GNU-stack warning.
No warning-free or memory-safety audit is claimed. Original lab inputs and the
standalone repack have separate artifact hashes because target names, shim debug
information and build paths differ.
