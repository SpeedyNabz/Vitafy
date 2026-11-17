> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Home playback with graphics gated

**Device result:** the user reports Home playback continues, returning works and ordinary closing works on home.1. Accepting a game-triggered forced close while playback runs still hangs the shell and requires a forced reset. Do not repeat that transition. This blocks release despite the Home improvement. The next [no-lease Home control](forced-close-investigation.md) tests ownership calls only; it is not a game force-close fix.

9 November 2026. The user reports:

- Recovery.1 closes/reopens normally.
- Attribute-control.1 closes, leaves other apps/Home folders usable, and reopens normally.
- On attribute-control.1, music stops at PS/Home and resumes when returning.

Attribute-control.1 and the failed relaunch.1 have identical executable bytes. Restoring ATTRIBUTE from 8 to 32768 contributes to the observed close regression. That comparison removes no-suspend-on-deactivation and restores the original communication-zone bit together; it does not identify which bit or interaction caused the failure. BGM ownership is not sufficient to prevent the current normal game app from being suspended at Home.

## Current experiment

VitaSpot-0.0.5-home.1.vpk (historical lab reference: `../artifacts/experiments/VitaSpot-0.0.5-home.1/VitaSpot-0.0.5-home.1.vpk`) adds a graphics lifecycle gate to the frontend and requests ATTRIBUTE=32776 (0x8008). This preserves the working base 0x8000 and adds only no-suspend-on-deactivation. It does not request disable-system-suspend, system app mode, a smaller heap, a shell plugin or a different app memory budget. Title ID and private account directory are unchanged. Package version is 01.08; the running app identifies itself as 0.0.5-home.1.

The gate queries the installed VitaSDK's _sceAppMgrGetAppState and sceCtrlPeekBufferPositive. It denies graphics when system UI overlays the app, input is intercepted by another app, or either query lacks a valid sample. It logs gate transitions in vam.log. Readability of these state signals in Home on the user's firmware is unverified.

While gated, the frontend continues polling/releasing backend events and reading player state, but skips image uploads, text rendering, GPU presentation, deferred texture destruction, UI input handling and its own dimming/backlight policy. The existing independent Rust networking/decoder/output threads remain unchanged. Returning to a valid foreground sample allows rendering again. The main loop also stops immediately on script/SDL quit rather than drawing one more frame after quit.

This addresses missing lifecycle handling found in the code. It is not a proven explanation of the earlier shell hang. There is still a transition race between sampling focus and submitting graphics; no atomic graphics-ownership guarantee is claimed. Closing through LiveArea may be a forced termination and may not execute the app's ordinary shutdown path.

## One Home/return/close check

Keep recovery.3 (historical lab reference: `../artifacts/recovery/VitaSpot-0.0.4-recovery.3/VitaSpot-0.0.4-recovery.3.vpk`) available first. It is the original working 0.0.4 executable/resources with only install version raised to 01.09, allowing replacement of this 01.08 experiment without a downgrade.

1. Install home.1 over VitaSpot through VitaShell, preserving ux0:data/VitaSpot/.
2. Confirm playback. Press PS/Home and listen for about a minute, including a track change if possible.
3. Return and check playback/display/controls. Then close once, check a Home folder and another app, and reopen only if the shell remains responsive.
4. Stop on any shell regression; do not repeat the failing cycle. Once the shell is responsive, restore recovery.3 if needed. Do not share credentials.json or unredacted vam.log.

Record continued music, gate transition log lines if accessible, return rendering and close/reopen outcome separately. If playback continues but closing still breaks the shell, this is a failed experiment. If the gate never blocks in Home, the current state signals are insufficient.

Do not test launching a game or pressing power as part of this first check. Game coexistence still requires a separate app-budget/service architecture. True hardware suspend still cannot execute the streaming client. Public release remains on hold for hardware lifecycle acceptance and the unresolved upstream frontend license.

## Repeatable local evidence

`prepare_home_sources.py` first verifies the three pinned patches reconstruct exactly. It creates or checks an isolated frontend tree under build/home-source, copies unchanged resources, and makes seven reviewed frontend/build edits. It writes a diff and SHA-256 inventory. It never edits the pinned client or its backend library. Edited source text is normalized to LF; unchanged file bytes are preserved.

`Test-HomeLifecycle.ps1` builds that tree with MSVC and the offline mock. It passed four integration assertions:

- The framebuffer stayed byte-identical during a 4.5-second simulated overlay.
- The framebuffer updated after returning; visual inspection showed the authenticated Home view, confirming mock login progressed during the pause in drawing.
- A foreground SDL_QUIT with a queued screenshot produced no extra frame.
- The same quit check passed while backgrounded.

Desktop screenshot readback while simulated-backgrounded is a test-only devscript operation; it is excluded from native builds. The host scenario does not exercise Vita focus delivery, Spotify network traffic, audio or physical close behavior.

`build_home_probe.py` requires passing host evidence for the exact source inventory, then builds the real native backend/frontend in build/home-native. It checks the native lifecycle symbols, ELF/SELF payload, resources and normal app budget. The native build passed with only the pre-existing SDK crtn.o GNU-stack warning. Rust dependencies were not changed/rebuilt; the linked backend archive hash is recorded.

It also creates a same-executable normal-ATTRIBUTE home-control.1 (01.07) for binary comparison, and recovery.3. The control is retained as comparison evidence; it is not the recommended package to install after home.1 because its install version is lower.

All five Home package tests passed: same executable for Home/control, exact original assets, exact working rollback executable, source/host evidence hashes, and native ELF/SELF/package identity. Debug ELF/VELF, diff, full build logs and host BMPs are retained locally. No device success is inferred from these checks.

| Artifact | SHA-256 |
| --- | --- |
| home.1 | c89815f491528f71d425a46a5269ec601c204c4cc35b94028f9ea8008de4785b |
| home-control.1 | 230d180513e1d17a4eeeed18998abb12048aabf141839d2d8ccc3c21f0627386 |
| recovery.3 | 878c9380f344783d7975f43f9c426c11f7c0e5092f9aff1dd8b6a9cb54cef49c |

## Primary references inspected

- [VitaSDK AppMgr declarations](https://github.com/vitasdk/vita-headers/blob/ebc8f4f7ac8313fe4f74ca95724116b224bad184/include/psp2/appmgr.h) and the installed appmgr.h: state layout/API. Installed ctrl headers define SCE_CTRL_INTERCEPTED as input unavailable due to another application.
- [GrapheneCt TransparentImpose](https://github.com/GrapheneCt/TransparentImpose/blob/d332be752042d2424315acfb6d2b0ab85587973d/TrImpose.c): version-zero AppMgr state call pattern.
- [VitaArc input implementation](https://github.com/TheSnowfield/VitaArc/blob/3d4f2f8f7597925bad78c1141d661f93e02d063d/src/platform/input.c): combined overlay/intercepted-input state pattern. No source was copied.
- [SDL2 Vita video implementation](https://github.com/libsdl-org/SDL/blob/SDL2/src/video/vita/SDL_vitavideo.c): inspected event pumping does not provide a general application background lifecycle bridge. This is a current reference, not an assertion that the installed binary matches every line.
- [PARAM.SFO research](https://www.psdevwiki.com/vita/PARAM.SFO): attribute meanings, with firmware/speculation caveats.
