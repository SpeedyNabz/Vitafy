> Archived lab handover. Read [current status](../status.md) first; no historical VPKs or raw device logs are distributed here.

> Current context, 17 November 2026: see [current findings](../research/current-findings.md) and the [open work plan](../open-work-plan.md). The historical results and instructions below retain their original scope; they are not new device-test requests.


# Scope and decisions

## User requirements and observed preferences

The conversation began with three workstreams: improve Codex-assisted production
delivery and choose useful plugins/skills; create a Vita app/game development lab;
and prepare a lab for researching/decompiling/modding the user's installed PC copy
of Midnight Club 2. The active implementation then became native Spotify playback
on a Vita that already runs homebrew. The user has Spotify Premium. The user later confirmed that all reported Vita/device testing used a real modded PS Vita PCH-1003.

"Runs a Spotify executable" was clarified to **native audio playback on the Vita**.
A desktop executable launcher, browser assumption or remote-only controller would
not satisfy that decision. Reusing VitaSpot provided a native frontend and real
Rust/librespot backend; the user subsequently confirmed the original application
worked perfectly. A phone may provide Spotify Connect pairing/control while the
Vita itself decodes and outputs the audio.

The desired background modes include Home, other apps, games and a short power-button
press if supported. The user also requested release finalization/cleanup. Display-off
operation and true hardware suspend are different questions; neither is established
by a keep-awake call or an idle heartbeat. No public publishing was requested.

The workspace is Windows/PowerShell. Continue concrete authorized work without
repeated permission questions. Ask for observations that only hardware testing can
provide, make tests bounded and focused, and preserve the user's working account.
No subagents were used in this work. Do not silently introduce parallel agents.

## Codex production-delivery workstream

Created codex-production-playbook (historical lab reference: `../../../../codex-production-playbook/README.md`)
with environment audit, sourced workflow research, prompt examples, plugin/skill
recommendations and templates for briefs, plans, quality gates and releases.
The core recommendations were a concrete user journey and acceptance criteria,
short project-specific instructions, a runnable vertical slice, repeatable checks,
real integration validation, preview/release/rollback evidence, and narrowly scoped
skills after a procedure has worked manually. These are engineering recommendations,
not measured guarantees about speed or success rate.

Historical recommendations included relevant GitHub/browser/design capabilities,
Playwright for repeatable web UI work, security review practices and gh-fix-ci when
CI exists. No curated skill installation was completed during that investigation.
Plugin availability/model recommendations were observed at the research date;
recheck before a new installation or current-product recommendation. The playbook
does not claim that every suggested dependency is installed or every repo audited.

## Vita lab

psvita-lab (historical lab reference: `../../../README.md`) contains general homebrew research, stack choices,
development guidance and toolchain notes. The active app is this vita-spotify
project. Toolchain installation/builds were completed here; the initial research
documents' statements that they were pending are historical.

## Midnight Club 2 lab

midnight-club-2-lab (historical lab reference: `../../../../midnight-club-2-lab/README.md`) contains a baseline
inventory tool, source register, existing-tool review, staged improvements,
compatibility matrix and targeted reverse-engineering plan. The user said the game
was already in the workspace. The located installation was Midnight Club II and
its root executable was mc2-mp.exe, not mc2.exe.

- Recorded executable SHA-256:
  `42669218991ecf9e0c90c2271353f2df3ae8a33c527f3a00e730f83365e43a0b`.
- Recorded PE32/x86 image base: `0x400000`.
- Initial full inventory: 214 files, no read errors/skipped files, stable reads.
- Distribution/build identity, patch/mod provenance and runtime compatibility were
  not established by the filename or hash alone.
- No game installation files were modified; no decompilation, working mod or full
  source reconstruction was completed in this conversation.

OpenMC2 was considered a partial, abandoned reference rather than a complete source
port. mc2hook was a candidate for reversible incremental hooks/assets, with exact
build compatibility still to be tested. A full matching decompilation is a separate
open-ended project. The preferred progression is fingerprint, preserve a baseline,
test in a sandbox, implement one reversible improvement and reverse engineer the
specific subsystem needed next. Proprietary binaries/assets remain user inputs.

The snapshot preserves these authored records without copying the game or private
inventory JSON. This Vita handover does not assert their current installation paths
still exist; the facts above describe the recorded investigation.
