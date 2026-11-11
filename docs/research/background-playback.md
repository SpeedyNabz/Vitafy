> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

# Background playback implementation and limits

Prepared 9 November 2026. The user selected Home/other-app playback and power-button playback if supported. The previous 0.0.4 VPK's core operation is user-reported working. The user reports that launching a game closes 0.0.5-rc.1 and stops playback; other transitions remain unconfirmed. A separate [gdc residency experiment](resident-probe.md) now tests executable boot budgets and a bounded heap without changing the frozen candidate.

## Inspected baseline

The Rust sink already opens SCE_AUDIO_OUT_PORT_TYPE_BGM (value 1). It did not call sceAppMgrAcquireBgmPort. Installed VitaSDK declares that function as obtaining the BGM port even when the application is not in front. Audio decoding/output already has independent Rust threads, so an app-resident implementation can continue without redrawing the UI.

## Changes

- Acquire AppMgr BGM ownership before opening the real audio port. If denied, retain foreground playback and log the failure rather than terminating or evicting another app.
- Track successful ownership; release it on sink stop/drop, failed audio-port open, and failed output-thread creation. Repeated stop is safe at the ownership/port layer.
- Tick only the automatic-suspend inactivity timer once per second from the running audio-output loop. UI presentation may block while deactivated, so the UI heartbeat alone is insufficient.
- Request ATTRIBUTE 0x00080008: no-suspend-on-deactivation and disable-system-suspend. These are experimentally applied package attributes, not proof of every firmware's behavior.
- Restore the panel brightness on normal platform shutdown. Preserve the existing screen-off shortcut, controls, app ID, account storage, and memory-budget defaults.

## Limits that need device evidence

Deactivation, display-off/lock, full hardware suspend, process closure, and launching another game are different states. A true suspended CPU cannot keep the client's decoder and network loop running. The candidate instead requests residency/avoidance of system suspend. That can affect battery and power-button behavior while the app is present, including when playback is paused; the hardware matrix tests this explicitly.

A second game may require closure of this application for memory/application-class reasons. A full shell-hosted Spotify streaming service would be a separate architecture. This change does not install a shell/kernel plugin, raise the app's memory partition, force another app closed, or claim survival after this process is terminated.

## Research decisions

[VitaSDK AppMgr declarations](https://github.com/vitasdk/vita-headers/blob/ebc8f4f7ac8313fe4f74ca95724116b224bad184/include/psp2/appmgr.h) are the source for ownership calls. [PARAM.SFO documentation](https://www.psdevwiki.com/vita/PARAM.SFO) describes the two selected flags and explicitly notes uncertainty for some firmware features. This is why hardware confirmation remains required.

[ElevenMPV's native player](https://github.com/joel16/ElevenMPV/blob/master/source/main.c) acquires/releases BGM ownership and its audio library uses a BGM port. [VitaWave](https://github.com/Jyotiraditya-Samal/Vitawave) supplies another public background-audio pattern, but its unrelated SFO flags and memory settings were not copied.

[libShellAudio](https://github.com/GrapheneCt/libShellAudio) exposes shell decoding for specific file codecs; adopting that file-player API would not directly host the current live librespot session. [vita-shell-companion](https://github.com/Breezyslasher/vita-shell-companion) describes a shell service but labels its generalized protocol untested on a console. It was not introduced as a release dependency. These comparisons are engineering judgments based on the inspected interfaces, not device benchmarks.

No code from those reference players/services was copied. Changes are captured in patch 0002; the bounded font-cache cleanup is patch 0003.
