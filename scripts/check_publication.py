"""Check the exact Git-index publication set, not ignored local account/build data."""
import re
import subprocess
from pathlib import Path
import hashlib
from common import ROOT, LOCK, sha256


def main():
    output = subprocess.check_output(["git", "-c", "safe.directory=" + ROOT.as_posix(),
              "-c", "core.excludesFile=", "-C", str(ROOT), "ls-files", "-z"])
    names = output.decode().rstrip("\0").split("\0") if output else []
    if not names: raise ValueError("Stage the intended source/documentation files first")
    failures = []
    secrets = re.compile(rb"(?:gh[pousr]_[A-Za-z0-9]{24,}|github_pat_[A-Za-z0-9_]{30,}|AKIA[0-9A-Z]{16}|-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----)")
    for name in names:
        path = ROOT / name
        parts = Path(name).parts
        if parts[0] in (".cache", "build", "artifacts", "private", "target") or path.suffix in (".vpk", ".elf", ".velf", ".self", ".a", ".o", ".obj", ".pyc", ".log") or "credentials" in path.name.lower():
            failures.append("Generated/private file staged: " + name)
        data = path.read_bytes()
        if secrets.search(data): failures.append("Credential-like content in: " + name)
        if re.search(rb"C:[/\\]Users[/\\]", data): failures.append("Machine-local user path in: " + name)
        if path.suffix == ".md":
            text = data.decode("utf-8-sig")
            for target in re.findall(r"!?\[[^\]]*\]\(([^)]+)\)", text):
                if "://" in target or target.startswith(("#", "mailto:")): continue
                destination = (path.parent / target.split("#", 1)[0]).resolve()
                try:
                    destination.relative_to(ROOT)
                    internal = True
                except ValueError:
                    internal = False
                if not internal or not destination.is_file(): failures.append("Broken documentation link: " + name + " -> " + target)
    for item in LOCK["patches"]:
        if sha256(ROOT / item["file"]) != item["sha256"]: failures.append("Patch checksum differs")
    for relative, expected in LOCK["overlays"].items():
        path = ROOT / relative if relative.startswith(("src/", "tests/")) else ROOT / "overlays" / relative
        if sha256(path) != expected: failures.append("Overlay checksum differs: " + relative)
        indexed = subprocess.check_output(["git", "-c", "safe.directory=" + ROOT.as_posix(), "-C", str(ROOT), "show", ":" + path.relative_to(ROOT).as_posix()])
        if hashlib.sha256(indexed).hexdigest() != expected:
            failures.append("Indexed overlay bytes differ: " + relative)
    if failures:
        raise ValueError("\n".join(failures))
    print("Publication checks passed:", len(names), "indexed source/documentation files; local links and known credential patterns checked")


if __name__ == "__main__":
    main()
