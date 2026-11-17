> Archived lab handover. Read [current status](../status.md) first; no historical VPKs or raw device logs are distributed here.

> Current context, 17 November 2026: see [current findings](../research/current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Current state and resume instructions

As of 11 November 2026. This documentation request is not a new device test result.

## Latest verified milestone

Real backend runtime 0.5 initialized, reached logged-out discovery, sampled idle
allocator state and shut down normally. Numeric logs and filtered backend diagnostics
support this. The user confirmed normal folders/ordinary apps afterward. Worker-only
LiveArea persistence had already passed by user observation. Audio was blocked in
0.5, so it did not validate music playback.

## Current outstanding device check

VitaSpot-ServiceProbe-0.6-playback.1.vpk (historical lab reference: `../artifacts/service-probes/VitaSpot-ServiceProbe-0.6-playback.1/VitaSpot-ServiceProbe-0.6-playback.1.vpk`)
is built and has three passing native/package checks. **No response to that playback
test has been received.** SHA-256:
`de219ff517b9ed2ca154e61a8dc41f487d97bf8316be23fb59a92074251e96f3`.

It requests the same startup-tested 48 MiB SELF/24 MiB heap, with worker persistence
bit 0x04000000. It enables actual audio/BGM ownership and appears as **VitaSpot
Service Test** in the phone's Spotify Connect picker. It uses a fresh isolated
playback-<PID> directory rather than the working app's account data.

Outstanding procedure, already supplied to the user:

1. Close VitaSpot manually; install over the separate Service Probe bubble.
2. Use the same Wi-Fi on phone/Vita; launch once and select VitaSpot Service Test
   in Spotify. Start music and confirm it comes from Vita speakers/headphones.
3. After roughly 30 seconds, skip to another track from the phone. Stay awake at
   Home. Do not remove LiveArea, launch a game or press power during this test.
4. The test should request pause/shutdown around four minutes after initialization,
   including pairing time. Check Home folders/another ordinary app afterward.
5. Inspect launcher.log (probe=6) and playback.log under
   ux0:data/VitaSpot-ServiceProbe/. Read playback-timeout.log if present. Ask for
   audible output, track skip, automatic stopping and shell-health observations.

The monitor reports a stall after about five minutes without forcibly killing an
audio owner. Loops are bounded by waits, not hard wall-clock guarantees. On a failure,
use existing logs and the narrowest observation; do not ask for repeated game-close
hangs or relaunch loops. Never ask for credentials.json or unfiltered backend logs.
The user supplied vam.log in an earlier test; only selected non-account diagnostics
were retained. A "playing" field alone is not proof of audible output.

## After playback evidence arrives

- If startup fails: classify launch rejection versus worker entry/runtime failure
  using the actual logs before changing unrelated flags or memory.
- If pairing/audio fails: check numeric auth/player transitions and sampled memory;
  inspect selected backend diagnostics locally without exposing account details.
- If playback works: verify controlled pause/shutdown and BGM release before adding
  closure, other-app or game transitions. Validate one meaningful new boundary at a time.
- Prove active-audio worker lifetime across the intended UI lifecycle; the earlier
  tiny no-audio page-removal pass is not enough.
- Implement a bounded, versioned IPC adapter and reconnecting Vita frontend. Keep
  backend ownership/allocations in the service; preserve the existing process-local
  ABI for mock/desktop work where useful.
- Test game coexistence only after service playback/lifecycle/resource evidence
  supports it. The original foreground game's force-close failure remains unresolved.
- Investigate display-off and true suspend separately; assess Wi-Fi and battery
  behavior. No power-button/sleep success is recorded.

## Working main-app alternatives and rollback

home.1-restored contains the exact Home-working home.1 executable with install
version 01.12; its installation was not independently confirmed. It retains the
known game-triggered hang. If used, close VitaSpot manually before a game, as the
user successfully did; do not accept a game prompt while it is playing.

Recovery.5 contains the original working foreground executable/resources with
install version 01.13. It is a preserved rollback, not a new service release.
Keep ux0:data/VitaSpot/ intact; do not uninstall/reset the main title or copy account
files between these test directories. Service probes update only their separate title.

## Release work remaining

No current package is accepted as the final requested background-playback release.
Production readiness still requires real service playback, frontend integration,
reconnection/Wi-Fi failures, reliable shutdown/relaunch, event/IPC ownership, bounded
resource use over a soak, active-audio lifecycle checks and supported background
modes. Review logging/privacy, install/update/rollback, versioning, release assets,
UI messages and controls. Remove experimental flags/code only through reviewed
changes backed by the relevant regression evidence.

The upstream frontend root lacks a blanket LICENSE file. Backend Cargo metadata
says MIT; fonts/vendored crates have separate notices. Resolve frontend/assets
redistribution terms before public distribution; do not assume a backend license
covers everything. The implementation uses unofficial Spotify protocols/internal
endpoints according to upstream; compatibility/approval is not guaranteed. No
public publish, external release or kernel/shell plugin installation has occurred.

The user confirms the test hardware is a real modded **PS Vita PCH-1003**, used for all reported Vita/device testing. Exact firmware and installed plugins remain unspecified. Do not infer them or compatibility with other models/firmware. Windows mock/build/package checks remain separate.

## New-session starting instruction

Read this handover, applicable AGENTS.md files and source-lock.json. Preserve current
client edits and frozen outputs. The next missing evidence is the 0.6 playback test;
do not repeat already-passed idle/runtime controls without a new reason. Do not mark
games, power/sleep or release complete based on those controls. Update the ledger and
evidence map when genuine new observations arrive.
