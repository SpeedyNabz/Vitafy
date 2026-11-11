> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

# Sources, provenance, and limitations

Reviewed 8 November 2026. Local source inspection used the exact commit in source-lock.json; website summaries may change after this date.

| Source | Use |
| --- | --- |
| [VitaSpot](https://github.com/0oPerryo0/VitaSpot) | Native client, build workflow, premium requirement, interface and platform code |
| [cspot_vita](https://github.com/michal4132/cspot_vita) | Older native client considered as a reference |
| [Spotify Web Playback SDK](https://developer.spotify.com/documentation/web-playback-sdk) | Official browser product; does not establish native Vita support |
| [Spotify authorization](https://developer.spotify.com/documentation/web-api/concepts/authorization) | Official authorization reference; no app registration or OAuth flow was implemented here |
| [VitaSDK](https://vitasdk.org/) / [vdpm](https://github.com/vitasdk/vdpm) | Native toolchain setup references |
| [SDL2 release](https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10) | Pinned Windows SDL2 development package |
| [SDL2_image release](https://github.com/libsdl-org/SDL_image/releases/tag/release-2.8.12) | Pinned Windows image dependency |
| [SDL2_ttf release](https://github.com/libsdl-org/SDL_ttf/releases/tag/release-2.24.0) | Pinned Windows font dependency |

The upstream source and dependency archives were downloaded into this project for local development. No release binary or credentials were downloaded, no Spotify service was contacted, and nothing was published.

## License and service compatibility findings

The inspected root has no top-level LICENSE file establishing a blanket frontend license. The backend Cargo.toml declares MIT, and individual vendored crates/fonts have their own metadata/notices. Do not assume one dependency's license covers the entire frontend. Clarify the frontend's redistribution terms before a public derivative release, preserve third-party notices, and avoid relabeling upstream code as entirely original work.

The upstream README explicitly says its private protocol/internal endpoints are unofficial and incompatible with Spotify's terms. That is a reported upstream limitation, not an official Spotify approval or a legal conclusion supplied by this project. Personal development does not establish public-distribution rights or future service compatibility.

The current work adds local build adapters, a recorded allocator fix, reproducible inputs, and validation. It does not add account workarounds, protection bypasses, or a music export feature. Premium is an upstream runtime requirement; it is not needed for the offline mock tests.
