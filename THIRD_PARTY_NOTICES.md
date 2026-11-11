# Attribution and licensing scope

Vitafy is derived from **VitaSpot by 0oPerryo0**:
https://github.com/0oPerryo0/VitaSpot

Baseline commit: `81c533f1ef61623360ea4fbe32cf29b4f2f3901f` (VitaSpot 0.0.4).
The source lock and patch preserve this provenance. Source preparation fetches
that revision into an ignored local directory; a full upstream source tree or
compiled application is not bundled in this repository.

The inspected upstream root has no top-level LICENSE establishing a blanket
frontend grant. Its backend Cargo manifest declares MIT, and vendored crates and
fonts have their own notices. No blanket MIT or other license is assigned to
upstream material or the entire application here. Clarification of frontend and
asset redistribution terms remains outstanding before a public binary release.

The patch includes the previously reviewed allocator pairing fix, BGM ownership
cleanup/keep-awake changes, bounded font-cache scan, graphics gating, and the new
Vitafy UI/notifications/reliability work. It does not claim the upstream client,
Spotify protocol implementation or dependencies as original Vitafy work.

Dependency/asset families include librespot and vendored Rust crates, SDL2,
SDL2_image, SDL2_ttf, FreeType, HarfBuzz, libpng, libjpeg-turbo, libwebp, bzip2,
zlib, VitaSDK runtime libraries, Figtree and Noto fonts. Their own manifests and
notices remain in materialized upstream source; the preserved font notices also
remain in the package overlay. The logo is a new code-drawn Vitafy mark.

Research consulted installed VitaSDK headers/stubs and public native samples,
including BGM/service examples. No shell/kernel plugin is installed. Technical
references and their limitations are documented in [research](docs/research-index.md).

Spotify's upstream private protocol/internal endpoint compatibility and service
terms limitations remain applicable. This is an unofficial project, not approval
from Spotify or Sony. See [historical findings](docs/research/sources-and-licensing.md).
