> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Native Vita build and deployment

Prepared 8 November 2026 and verified on 9 November 2026. This sequence has now produced a real-backend Release VPK on this Windows host.

VitaSDK, target libraries, and the pinned Rust nightly/rust-src are installed. The ARM compiler/SELF smoke test and the full native build passed. Use the [configured toolchain guide](toolchain-setup.md) for current paths and versions.

## Required environment

1. VitaSDK with compiler, CMake helpers, and the libraries below. Select a dedicated install location and record the exact SDK/channel/package state. Do not reuse the desktop SDL VC packages for the ARM target.
2. Rust nightly-2026-10-18 with rust-src. This is the nightly pinned by the inspected upstream CI, not an independently verified local cross-build.
3. CMake and Git Bash for the provided Windows wrapper. For Linux/WSL, use the upstream Bash scripts directly.

Required target libraries from the pinned workflow:

```text
sdl2 sdl2_ttf sdl2_image freetype harfbuzz libpng
libjpeg-turbo libwebp zlib bzip2
```

The native linker also needs the SDK's pthread and platform stubs. Upstream's script builds its POSIX shim and links ring's ARM assembly archive. Do not substitute the desktop Windows library or omit these steps.

## Current host observations

- Visual Studio 2022 Build Tools exists; the desktop build succeeded with MSVC 19.44.35228.0 and Windows SDK 10.0.26100.0.
- CMake, Git Bash, rustup, and stable x86_64-pc-windows-msvc Rust are present.
- VitaSDK 2026.08.1-1 and nightly-2026-10-18 with rust-src are installed and verified through the native build.
- Build-Vita.ps1 builds the real backend, native C frontend, and VitaSpot.vpk successfully with Ninja and Release settings.
- Earlier WSL enumeration was denied and Docker was unreachable in this session; neither route is assumed to work.

## Preparing the toolchain

Use the current official [VitaSDK](https://vitasdk.org/) and [vdpm bootstrap](https://github.com/vitasdk/vdpm) documentation. A native PowerShell bootstrap exists; verify its checked-out source and published artifact/hash before running it. Prefer an install under a dedicated development-tools folder. Record an exact installer revision and SDK state in source-lock.json once it has been demonstrated.

The Rust installation command used by upstream is:

```powershell
rustup toolchain install nightly-2026-10-18 --profile minimal --component rust-src
```

This command was executed on 9 November 2026. The project wrapper and rust-toolchain.toml select the nightly for this project; the user's global default remains stable.

After SDK setup, verify the compiler and vdpm status, install the required target packages, and first build a small upstream SDK sample. Then run from the project root:

```powershell
.\scripts\Initialize-Desktop.ps1 -Offline
.\scripts\Build-Vita.ps1
```

The wrapper invokes the pinned client's inspected build-vita.sh then build-vpk.sh. Expected output: client/build-vita/VitaSpot.vpk. It prints a SHA-256 only after the package exists. Preserve the matching symbols, SDK status, Cargo.lock, source pin, and patch manifest with the build.

For Linux/WSL, set VITASDK and VAM_RUST_TOOLCHAIN=nightly-2026-10-18, then use those same upstream Bash scripts from client/backend/librespot-vam. The Windows wrapper specifically checks native Windows SDK paths; it is not a WSL launcher.

## Device deployment and acceptance

The user's Vita already runs homebrew. Once a real-backend VPK exists, install through VitaShell and follow the pinned client's sign-in instructions. Upstream describes same-network Spotify Connect sign-in, with a PC login helper as a fallback. Keep any generated credentials local; do not paste them into chat or include them in a build package.

Upstream uses title ID VTAM00001 and data path ux0:data/VitaSpot/. Check for an existing installation and preserve its settings/account data before installing a development package with the same identity. A separately branded development variant should use a deliberate distinct identity and matching storage paths.

Pass criteria: audible local output, correct controls/volume, track transitions, seek/pause/resume, reconnect after network loss, account logout, and stable lifecycle behavior. A Connect device listing alone is not playback acceptance.
