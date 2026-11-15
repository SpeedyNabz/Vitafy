"""Refresh the generated tree from reviewed canonical overlays and update hashes.

Explicit maintenance action, never used implicitly by the build. Existing locked
files must verify before changing overlay hashes, unless --accept-overlay-edits
was deliberately passed after editing canonical source files.
"""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--accept-overlay-edits', action='store_true', required=True)
    parser.parse_args()
    lock = json.loads((ROOT/'source-lock.json').read_text())
    names = set(lock['overlays'])
    for folder in ('src','tests','overlays'):
        for path in (ROOT/folder).rglob('*'):
            if path.is_file():
                names.add(path.relative_to(ROOT/'overlays' if folder=='overlays' else ROOT).as_posix())
    for name in sorted(names):
        origin = ROOT/name if name.startswith(('src/','tests/')) else ROOT/'overlays'/name
        data = origin.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        lock['overlays'][name] = digest
        lock['prepared_file_sha256'][name] = digest
        target = ROOT/'build/source'/name
        target.parent.mkdir(parents=True,exist_ok=True)
        target.write_bytes(data)
    (ROOT/'source-lock.json').write_bytes((json.dumps(lock,indent=2)+'\n').encode())
    (ROOT/'build/source-verification.json').write_bytes((json.dumps(dict(upstream=lock['upstream'],patch_sha256=lock['patches'],files=lock['prepared_file_sha256']),indent=2)+'\n').encode())
    print('Updated canonical overlay hashes and generated source')

if __name__=='__main__': main()
