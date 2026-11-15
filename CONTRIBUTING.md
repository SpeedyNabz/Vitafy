# Contributing

Read the README, current status and research relevant to your change. Keep domain
and notification logic separate from Vita rendering/input/storage adapters.
Record the actual SDK/upstream revisions and validate declarations and link stubs.

Maintain authored new code in `src/` and `tests/`; maintain existing frontend and
backend changes in the canonical `overlays/` and reviewed
`patches/0001-vitafy.patch`. Run
`python scripts/refresh_sources.py --accept-overlay-edits` after deliberate canonical
changes. Generated `build/source/` is for
compiling, not the source of truth. Update reviewed hashes in `source-lock.json`
when intentionally changing a patch/overlay; verify a clean reconstruction.

Run `python scripts/build.py desktop`. For native changes, run the real native
build and preserve ELF/VELF and hashes locally. Distinguish mock tests, native
compilation, emulator observations, user reports and actual hardware checks.
Do not report unexecuted checks as passing or infer audio from a numeric state.

Never commit generated packages, caches, SDKs, account files, raw device logs or
private data. Do not request credentials in issues. Describe repro steps and
numeric/redacted diagnostics. Do not repeat the known unsafe game-close test.

Changes must preserve upstream attribution and dependency/font notices. Do not
apply a blanket license to upstream code; see THIRD_PARTY_NOTICES.md.
