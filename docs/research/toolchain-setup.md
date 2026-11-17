> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Configured native toolchain

Configured 9 November 2026 for this project. The user-wide Rust default remains stable; rust-toolchain.toml selects the pinned nightly inside this project.

| Component | Installed value |
| --- | --- |
| VitaSDK location | <lab>\.toolchain\vitasdk |
| SDK channel | 2026.08 |
| Channel sequence | 2026102473 |
| Toolchain snapshot | sdk-snapshot-20260825.611.1 |
| Core package | vitasdk-core 2026.08.1-1 |
| Target package snapshot | packages-2026.08-snapshot-20261024.61.1 |
| ARM C compiler | arm-vita-eabi-gcc 15.2.0 |
| Rust toolchain | nightly-2026-10-18-x86_64-pc-windows-msvc |
| Rust compiler | 1.101.0-nightly (8006091eb 2026-10-17) |
| Rust components | cargo, rustc, host rust-std, rust-src |
| ARM target | armv7-sony-vita-newlibeabihf, std built from source |

Installed target libraries: SDL2 2.32.8-1, SDL2_image 2.8.12-1, SDL2_ttf 2.24.0-1, freetype 2.14.3-1, harfbuzz 14.5.0-1, libjpeg-turbo 3.2.0-1, libpng 1.6.58-1, libwebp 1.6.0-1, bzip2 1.0.8-1, and zlib 1.3.2-2.

## Use in a fresh terminal

From this project:

```powershell
# The build script reads the configured SDK path automatically.
.\scripts\Build-Vita.ps1

# Or configure a terminal session for direct SDK commands:
. .\scripts\Use-VitaEnvironment.ps1
arm-vita-eabi-gcc --version
vdpm status
```

The environment helper changes only its process/session. No permanent user PATH or machine PATH change was made. The build wrapper restores its environment after finishing and uses Ninja for cross-compilation.

The Vita Rust target has no precompiled standard library. The pinned upstream build uses -Z build-std=std,panic_abort with rust-src. [Rust target documentation](https://doc.rust-lang.org/rustc/platform-support/armv7-sony-vita-newlibeabihf.html)

## Installer provenance and Windows adjustments

The official vdpm source was pinned at c9ba33bd428ba88052f79d78592f4b318a7a322b. Install-VitaSdk.ps1 creates a local adapted copy of its bootstrap: .NET Framework's IsPathRooted replaces unavailable IsPathFullyQualified calls; HTTP downloads use basic parsing; directory moves/cleanup are checked against the workspace before execution. Upstream source remains unchanged. Source/generated-script SHA-256 values are recorded in artifacts/toolchain/installer.json.

The bootstrap verified its archive checksum and channel metadata before installing. SDK/package status is saved in artifacts/toolchain/sdk-status.txt and sdk-packages.txt. These are generated local evidence, while source-lock.json records the selected versions.

Windows could not create three libpng package symlinks. The package archive was inspected to confirm each exact target; Repair-VitaSdkWindowsAliases.ps1 supplies hash-matching regular copies of libpng-config, libpng.a, and libpng.pc from their libpng16 equivalents. It refuses to overwrite differing content. Run that helper again after reinstalling this package if the aliases are missing.

## Verification

The CMake/Ninja smoke project compiled an ARM ELF, converted it to a Sony ELF, and produced eboot.bin using this SDK. Its ELF SHA-256 was 4c65f81970e83aeb6568bcc03b9d94e79af48a92040f61ea67bb27a3b38b21a2; SELF SHA-256 was 8bcf6599536eef3dac10d403e495ca314566400ac1dfed882899aa2d5a64f392. The sample was not run on a device.

The native app's build result is recorded separately in validation.md. A successful compiler smoke test alone does not establish Spotify playback.
