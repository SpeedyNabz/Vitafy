> Historical lab research, 8–11 November 2026. Read [current status](../status.md) first. Paths referring to lab artifacts are historical references, not included downloads.

> Current context, 17 November 2026: see [current findings](current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Milestones

| Milestone | Deliverable and acceptance | Status |
| --- | --- | --- |
| Native route selected | Playback on Vita; inspected source and architecture | Complete |
| Reproducible local baseline | Exact source/package/patch lock, offline setup, Windows build | Complete |
| Frontend and C interface | Contract scenarios plus rendered UI lifecycle | Complete with mock backend only |
| Native toolchain proof | ARM ELF/SELF smoke builds; SDK status and Rust pin recorded | Complete, 9 November 2026 |
| Real VPK | Rust backend, compatibility shim, ARM frontend, package and hash | Complete, Release package structurally checked |
| Account integration | Premium sign-in and real metadata without leaking credentials | Pending |
| Baseline hardware playback | User reported the 0.0.4 application working perfectly | User-confirmed core operation |
| Background candidate | Ownership, heartbeat, package flags, cleanup, native build and local checks | Complete locally; device transitions pending |
| Release preparation | Candidate bundle, installation/rollback, notices, source/packaging checks | Complete locally; public frontend licensing unresolved |

## Proposed first device experiment

Install a real-backend build, select the Vita as the playback device using the upstream-supported flow, and play one track through the Vita's own output. Record time to sign-in, time to first sound, underruns, and whether the phone can stop acting as a controller while playback continues.

The user has now confirmed baseline playback. Use the new hardware matrix (historical lab reference: `../release/HARDWARE_TESTS.md`) for background transitions rather than repeating every initial setup step.

## Required failure scenarios

- No Wi-Fi, reconnect after loss, or discovery blocked by the local network.
- Logged-out state, expired credentials, and account without Premium.
- An unavailable track, an empty library, and slow metadata/image requests.
- Repeated seek/pause/skip, volume limits, and long playback.
- Closing the app, restarting, sleep/wake, screen-off behavior, and logout.
- Fresh installation and a development update with existing app data.

Keep game modding and this app's source independent. They share development practices, not runtime components.
