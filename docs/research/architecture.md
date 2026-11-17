> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Architecture and selected starting point

## Native playback path

Vita inputs and touch -> C/SDL2 frontend -> plain-C backend contract -> Rust/librespot backend -> Vita network and audio platform code -> local speakers/headphones.

This is a native homebrew executable compiled for the Vita. The project does not attempt to execute desktop x86 code or Android binaries.

The inspected upstream implementation separates:

- client/src: UI, navigation, text, image decode work, player screens, preferences.
- client/include/vam_backend.h: API version 3; authentication, requests, playback, and asynchronous events.
- client/backend/librespot-vam: real playback/network backend, vendored Vita portability changes, and audio sink.
- client/backend/mock: deterministic synthetic content for offline development.
- client/src/platform: Vita and desktop adapters.

The C ABI states that UI calls must return promptly; network work lives on backend threads. Backend-owned event/page/image allocations are released through vam_event_release, not directly by the UI. Strings are bounded UTF-8 buffers. Rust representations must preserve C layout and integer widths.

## Why this source

VitaSpot already contains the platform-specific work required for native playback, including Vita networking, audio output, Rust/newlib compatibility changes, Connect discovery, and a frontend/backend test contract. Reusing that implementation is a more direct starting point than a new protocol implementation. That is an engineering judgment; no device comparison benchmark has been run.

The older [cspot_vita](https://github.com/michal4132/cspot_vita) also targets Spotify playback, but its current service behavior was not verified. It remains a research reference rather than a second dependency.

Spotify's [Web Playback SDK](https://developer.spotify.com/documentation/web-playback-sdk) targets browser playback. It does not establish support for the Vita browser or supply a native Vita executable. Spotify's Web API authorization and playback-control endpoints are not a drop-in native audio decoder. A web-controller implementation would not satisfy this project's selected local-audio requirement.

## Local Windows adapter

cmake/desktop builds the same frontend sources with the mock backend and pinned official SDL DLLs using existing Visual Studio Build Tools. It avoids altering upstream's GCC/pkg-config build configuration. This lets us iterate on screens and the C interface on the current host, while keeping cross-compilation a separate step.

## First local patch

The Windows UI smoke test reproduced exit code 0xC0000374 (heap corruption). Inspection identified SDL_strdup allocations released through the C runtime's free in the text and image caches. patches/0001-match-sdl-string-allocators.patch replaces those deallocations with SDL_free. Ordinary malloc buffers remain paired with free.

After the patch, the same scripted lifecycle completed successfully and rendered the expected home screen. The patch also applies to the native source, but its hardware behavior remains untested. The reproduction and successful rerun establish the local regression result, not a complete memory-safety audit.
