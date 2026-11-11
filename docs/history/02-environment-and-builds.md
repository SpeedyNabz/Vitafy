> Archived lab handover. Read [current status](../status.md) first; no historical VPKs or raw device logs are distributed here.

# Environment and reproducibility

Device testing: user-confirmed real modded **PS Vita PCH-1003**. The Windows toolchain below builds and checks artifacts; its mock tests are not physical-device playback tests.

## Pinned inputs

| Item | Recorded value |
| --- | --- |
| Active project | <lab>/apps/vita-spotify |
| Native source | 0oPerryo0/VitaSpot, commit 81c533f1ef61623360ea4fbe32cf29b4f2f3901f |
| Original upstream version | 0.0.4 |
| Frontend/backend | C11 + SDL2 / Rust + librespot; C ABI version 3 |
| VitaSDK location | <lab>/.toolchain/vitasdk |
| SDK channel/sequence | 2026.08 / 2026102473 |
| SDK snapshot/core | sdk-snapshot-20260825.611.1 / 2026.08.1-1 |
| Package snapshot | packages-2026.08-snapshot-20261024.61.1 |
| ARM compiler | GCC 15.2.0 |
| Rust toolchain | nightly-2026-10-18-x86_64-pc-windows-msvc, with rust-src |
| Rust compiler | rustc 1.101.0-nightly (8006091eb, 2026-10-17) |
| Rust target | armv7-sony-vita-newlibeabihf, build-std |
| Host build | Visual Studio 2022 Build Tools/MSVC; Ninja for native CMake |
| Target SDL libraries | SDL2 2.32.8; SDL2_image 2.8.12; SDL2_ttf 2.24.0 |
| Windows SDL development archives | SDL2 2.32.10; image 2.8.12; ttf 2.24.0 |

Rust is selected locally for the project; the user's global stable default was not
changed. Native artifacts retain ELF/VELF debug evidence. Source-lock.json is the
pin register. Its release.version/attribute still describe the withdrawn RC1;
release.status and current device records describe the later experiments.

## Proven repairs and patches

| Patch | SHA-256 | Finding |
| --- | --- | --- |
| 0001-match-sdl-string-allocators.patch | 50e7cdb3b0a2d0a75e94cbdc496b82e49f871763bb44a4bd61f2f8f567412be7 | SDL_strdup strings must be released with SDL_free; fixed reproduced desktop heap corruption |
| 0002-background-audio-and-release-candidate.patch | 34d8a15299ec8b6e2cb424358edac9b25ac44d3590c2d006f73d6da76b7da607 | BGM ownership, sink cleanup and keep-awake, display cleanup and experimental RC attributes; package not promoted |
| 0003-bound-font-cache-scan.patch | 1f2fea2f48eea0510f3469fd2b5d2eb2149f07cef2b87e36ad8b6f7405e8a4e5 | Bound the font-cache array scan |

The canonical client has five modified tracked files: CMakeLists.txt,
backend/librespot-vam/src/platform/vita.rs, src/platform/platform_vita.c,
src/gfx/images.c and src/gfx/text.c. Home/service probes use isolated build copies
or separate source directories. Do not apply their modifications blindly to client/.

The retained real backend archive used by home.1 and service memory/runtime/playback
diagnostics hashes to
`71a7f505f9087e90b5b39b703251c9f4be6270771a9449621328e1ab018de5f0`.

## Build and validation workflow

From the project directory:

```powershell
.\scripts\Use-VitaEnvironment.ps1
.\scripts\Test-Desktop.ps1
.\scripts\Smoke-Preview.ps1
py scripts/audit_service_backend.py
py -m unittest discover -s tests -p test_service_playback_probe.py
```

Read the individual script first. Versioned probe builders deliberately refuse to
overwrite retained releases; running an already-used builder is not a rebuild recipe
for a new version. package_release.py reconstructs the pinned source plus reviewed
patches and checks exact source/package content. Common.ps1's individual reverse-patch
check needs care if future patches overlap. Build-Vita.ps1 uses the canonical client
configuration, whose historical RC1 attributes are not a new safe recommendation.

## Windows/tooling lessons

- Prefer rg; PowerShell does not expand file globs passed as literal rg path
  arguments. Use a directory plus `-g` patterns.
- UTF-8 BOMs exist in some authored records. Preserve them when changing text;
  a patch matching the apparent first line may fail because of the BOM.
- Keep source/metadata transformations exact and checked. In one CMake override,
  a Windows backslash path produced a `\U` escape error; forward-slash paths fixed it.
- SDK libpng symlink aliases were repaired as hash-identical regular files.
- A native process can write warnings to stderr and still succeed. PowerShell
  helpers use the exit code, not stderr alone. SDK crtn.o GNU-stack warnings were
  nonfatal, not proof of a clean warning audit.
- Compiler encodings vary: a checked wrapper's -1 return compiled as
  `mov.w r0,#4294967295`, equivalent to a `mvn r0,#0` form. A too-specific test was
  corrected; the VPK did not need a code change.
- Restricted execution/network may require a narrowly scoped escalation. No
  automatic-review rejection remains unresolved in this conversation.
- For the nested Git checkout use per-command safe.directory; do not change global
  Git configuration. Use an empty core.excludesFile override, not NUL.
- Copy supplied diagnostic files promptly: Downloads files disappeared between
  inspection and archival during one parent-only test. Preserve extracted findings
  honestly when raw data is no longer available.

These are recorded practices/results, not a fresh machine-wide tool inventory.
