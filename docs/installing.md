# Install, update and device acceptance

The [0.1.0 release](https://github.com/SpeedyNabz/Vitafy/releases/tag/v0.1.0)
provides the user-accepted Home 01.29 package. It updates the existing main player
bubble, title **Vitafy**, title ID `VTAM00001`.

| Package/source | Home APP_VER | Foreground configuration | Evidence |
| --- | --- | --- | --- |
| Released 0.1.0 / preview.7 | 01.29 | 01.30 | General Home acceptance by user report |
| Development 0.1.1-preview.1 | 01.31 | 01.32 | Host/native checks pass; Bluetooth/Next device acceptance pending |

The development source builds the latter; check out tag `v0.1.0` to reproduce the
released source. A newer install version is not itself proof of a tested release.
It does not replace the separate Service Probe title.

The Vita data path stays `ux0:data/VitaSpot/` so saved sign-in, preferences and
cache are preserved. Keep a private backup before installing. Do not uninstall or
reset the title as part of the rename, and do not publish account files.
Manually close the current player before installing through your normal homebrew
package workflow. Neither build script installs automatically.

1. Launch with the Vita awake and foreground. If pairing is needed, use the same
   Wi-Fi as your phone/computer and select **Vitafy PS Vita** in Spotify Connect.
   Never provide passwords or tokens through a GitHub issue.
2. Confirm sound actually comes from Vita speakers/headphones. Test play/pause,
   track skip, seek, volume, queue, library, saving and search.
3. Change tracks and check the song/artist toast and the Vita's notification
   history. Check long, accented, non-Latin and supplementary Unicode titles.
   Turn Settings → Notifications Off and verify subsequent notifications stop;
   relaunch and verify the preference persists.
4. Pause, then use Settings → Exit Vitafy for an orderly backend shutdown.
   Check shell folders/apps afterward and relaunch. Report audible output,
   behavior and numeric error codes separately from package/build success.

Do not launch a game while the player is active, accept game-triggered forced
closure during audio, or treat the screen-off shortcut as true sleep support.
Those transitions remain outside the preview's verified behavior.

For development-candidate acceptance, use the
[Bluetooth/skip checklist](bluetooth-volume-and-skip-buffering.md#device-acceptance).
App/Spotify volume is independent of physical speaker/headset levels in that
candidate. Record actual skip latency, route behavior and shell health; do not
infer sound from UI state. No candidate installation or new hardware test was
performed during the documentation update.

Game testing is blocked on the [service gates](open-work-plan.md#cross-app-playback-gates).
Historical Service Probe instructions are not a new recommendation to install or
run an old probe. Record model, firmware/plugins, exact package/hash, audio route,
quality, network conditions, scenario and measured/user-observed result separately.
Keep credentials, private titles/IDs and raw backend logs out of shared evidence.

The historical lab retains exact original foreground/Home rollback packages;
they are not redistributed here. Their hashes and evidence are described in the
archived research. Preserve your known-good local package and private data backup.
No downgrade, rollback, hardware recovery or installation was executed while
publishing this repository.
