> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Home works; game-triggered forced close still hangs

**Updated result:** the successful close was directly from Home while music was playing, followed by successful reopen. home.2 loses Home audio; returning/ordinary closing still work. The lease-removal approach is not promoted. See [Home restoration and the separate-worker lifetime preflight](service-lifetime.md) for the current direction. Do not repeat a game-triggered force close.

9 November 2026: the user reports that home.1 plays at Home, returns normally and closes normally. However, accepting a forced close to launch a game while playback is still running hangs the shell and requires a forced reset. This is a failed mandatory lifecycle scenario. home.1 is not release-qualified, and game-triggered force-close testing is suspended.

The user clarified that music was playing, they went to Home, closed VitaSpot directly there and reopened successfully. The game-launch transition remains the failed path. Do not repeat that transition to collect more evidence.

## Completed bounded comparison: home.2

VitaSpot-0.0.5-home.2.vpk (historical lab reference: `../artifacts/experiments/VitaSpot-0.0.5-home.2/VitaSpot-0.0.5-home.2.vpk`), package version 01.10, keeps the exact verified home.1 frontend source overlay, real Rust backend archive, graphics gate, normal app budget and ATTRIBUTE=32776. It changes the version label to home.2 and adds linker wrappers that deny AppMgr BGM ownership by build policy. No system acquire/release call is made. The existing BGM-type PCM audio port and audio-thread inactivity tick remain.

This tests whether Home audio can work without explicit system-wide ownership. It does not establish that the ownership calls caused the forced-close hang. The issue could also involve graphics resources, device/firmware behavior or interactions during forced termination. A process terminated by the OS is not guaranteed to run its ordinary C shutdown path or Rust destructors; rearranging that path alone cannot guarantee cleanup during a forced kill.

The wrapper returns the SDK's negative NOEXEC result. The backend should therefore take its existing foreground-fallback branch and keep bgm_acquired false. A policy log line explicitly identifies the intentional denial. The following generic backend message that ownership is unavailable is expected; it is not evidence that another music app occupies the slot. An attempted release is logged as unexpected and also performs no system ownership operation.

## Original comparison procedure (completed; not a new request)

Keep recovery.4 (historical lab reference: `../artifacts/recovery/VitaSpot-0.0.4-recovery.4/VitaSpot-0.0.4-recovery.4.vpk`) available before installing the control. It restores the exact original working 0.0.4 executable/resources, with only install version raised to 01.11 so it can replace home.2. Account data stays at ux0:data/VitaSpot/.

1. Install home.2 over VitaSpot and start music.
2. Press PS/Home for about a minute, return, and check whether music continued.
3. If the shell remains responsive, return to VitaSpot before closing it normally. Record whether playback was playing or paused at close.
4. Check a Home-screen folder and one other app. Stop on any regression; restore recovery.4 once the shell is responsive if needed.

Do not launch a game, accept a game-related close prompt, or test the power button in this comparison. Do not repeatedly induce a lockup to generate logs. Existing logs can be retained if accessible; never share credentials.json or unredacted account identifiers.

Home playback without the lease would justify removing that dependency from the candidate. Even if this check passes, safe game force-close is still unverified. If Home audio stops, the current platform path may require the lease; a separate resident player/service or system-app design needs further work.

## Local checks and scope

The builder verifies that the source overlay and real backend archive match home.1. The four earlier mock lifecycle assertions apply to that identical frontend only; they do not exercise the new Vita ownership policy. No mock run is represented as Spotify or Vita audio validation.

Native Release compilation passed. The linker wraps both AppMgr BGM functions, and nm confirms their actual system symbols are absent while both policy wrappers are present. Wrapper disassembly, SDK/Rust/backend hashes, ELF hash and overlay evidence are retained in ownership-verification.json. No Rust backend changes, new worker thread, kernel plugin, system memory increase or boot-mode change were introduced.

Four package/binary tests passed: unchanged Home metadata/assets except install version and executable version label/policy, actual ELF ownership-symbol exclusion, matched source/host evidence, and byte-identical working rollback payload. The only build warning was the existing SDK crtn.o GNU-stack note.

- home.2 SHA-256: 548df8f1fb0711d123c9a295d358e239481dd73117506d99c4d08e7e355efbc2.
- recovery.4 SHA-256: bcbbd55d29bfab011fe5d8501c8ca2d819db550901dd7756906091bff5ba4f98.

Primary API references: installed VitaSDK appmgr.h and audioout.h, plus the upstream [AppMgr declarations](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/appmgr.h) and [blocking PCM output API](https://github.com/vitasdk/vita-headers/blob/master/include/psp2/audioout.h). API declarations support the interface assumptions, not a diagnosis of the shell hang.
