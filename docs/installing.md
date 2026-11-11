# Install, update and device acceptance

No public VPK release is attached. A locally built preview can update the existing
main player bubble: title ID `VTAM00001`, APP_VER `01.14`, title **Vitafy**.
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

The historical lab retains exact original foreground/Home rollback packages;
they are not redistributed here. Their hashes and evidence are described in the
archived research. Preserve your known-good local package and private data backup.
No downgrade, rollback, hardware recovery or installation was executed while
publishing this repository.
