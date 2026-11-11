"""Shared, path-independent build helpers. No account data is read here."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent.parent
LOCK = json.loads((ROOT / "source-lock.json").read_text(encoding="utf-8-sig"))


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command, **kwargs):
    print("Running:", command[0], flush=True)
    return subprocess.run([str(x) for x in command], check=True, **kwargs)


def git(directory, *args, capture=False):
    command = ["git", "-c", "safe.directory=" + Path(directory).resolve().as_posix(),
               "-c", "core.excludesFile=", "-c", "core.autocrlf=false", "-C", str(directory), *args]
    if capture:
        return subprocess.check_output(command)
    return run(command)


def write_json(path, data):
    Path(path).write_bytes((json.dumps(data, indent=2) + "\n").encode("utf-8"))
