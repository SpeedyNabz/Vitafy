# Roadmap

Updated 17 November 2026. The maintained task list is the
[open work plan](open-work-plan.md). It records priorities, dependencies, exit
evidence and deferred features; unchecked items remain open. See
[current findings](research/current-findings.md) for the research behind it.

| Order | Outcome | Open work |
| --- | --- | --- |
| Now | Accept the Bluetooth/skip test build and classify remaining stalls | A1-A2: device route/volume/latency tests; blocking IO and underrun diagnostics |
| App reliability | Reconcile account state, recover playback and measure cache/large orders | A3-A5: live hearts/edits/handoff, Wi-Fi recovery, resource/cache stress and UI acceptance |
| Next app release | Promote only an accepted candidate with exact provenance | A6: 0.1.0 stays unchanged while 0.1.1-preview.1 awaits device acceptance |
| Service feasibility | Reproducible current worker within a measured budget; real audio and clean stop | B0-B1: old idle/heartbeat observations do not prove streaming feasibility |
| Service integration | Audio survives normal UI detach and reconnects through bounded IPC | B2-B3: one playback owner, versioned values, acknowledgements and backpressure |
| Coexistence | Handle contention and prove supported app/game transitions | B4-B5: no known-unsafe forced-close replay; publish an explicit compatibility matrix |
| Qualification | Power states, sustained playback, recovery and update/rollback | B6: separate display-off/suspend; archive measured evidence and exclusions |
| Later | Supported discovery/device controls and additional input polish | C1-C3 after reliability work |
| Deferred | Offline/download, video and wider social features | C4-C5 need separate service/entitlement/product work |

Released 0.1.0 promotes the user-accepted preview.7 Home binary unchanged.
Development 0.1.1-preview.1 separates app gain from hardware volume, preloads one
next song earlier, cancels obsolete async loads and rejects stale playback events.
Host/native checks pass; actual Bluetooth and skip improvement is not yet measured.

Home playback is not game compatibility. The historical worker passed idle
startup/shutdown and a tiny no-audio LiveArea lifetime test; its real-audio test
has no recorded acceptance. Current service resource limits remain unmeasured.
Playback across every app/game is a target subject to platform constraints,
not a capability inferred from flags or an existing release claim.
