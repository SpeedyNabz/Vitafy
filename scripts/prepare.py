"""Materialize the pinned upstream plus reviewed Vitafy changes, locally only."""
import argparse
import io
from pathlib import Path, PurePosixPath
import tarfile
from common import ROOT, LOCK, git, sha256, write_json


def verify(source):
    for relative, expected in LOCK["prepared_file_sha256"].items():
        path = source / relative
        if not path.is_file() or sha256(path) != expected:
            raise ValueError("Prepared source differs: " + relative)


def prepare(upstream=None):
    source = ROOT / "build/source"
    if source.exists():
        verify(source)
        print("Verified existing prepared source:", source)
        return source
    checkout = Path(upstream).resolve() if upstream else ROOT / ".cache/upstream"
    if not checkout.exists():
        checkout.mkdir(parents=True)
        git(checkout, "init")
        git(checkout, "remote", "add", "origin", LOCK["upstream"]["repository"])
        git(checkout, "fetch", "--depth", "1", "origin", LOCK["upstream"]["commit"])
        git(checkout, "checkout", "--detach", LOCK["upstream"]["commit"])
    revision = git(checkout, "rev-parse", "HEAD", capture=True).decode().strip()
    if revision != LOCK["upstream"]["commit"]:
        raise ValueError("Upstream checkout revision differs from source-lock.json")
    for patch in LOCK["patches"]:
        if sha256(ROOT / patch["file"]) != patch["sha256"]:
            raise ValueError("Reviewed patch checksum differs")
    data = git(checkout, "archive", "--format=tar", revision, capture=True)
    source.mkdir(parents=True)
    # Extract regular files/directories only, rejecting all traversals and links.
    with tarfile.open(fileobj=io.BytesIO(data)) as archive:
        for member in archive:
            name = PurePosixPath(member.name)
            if name.is_absolute() or ".." in name.parts or "\\" in member.name or ":" in member.name:
                raise ValueError("Unsafe upstream archive path")
            target = source / member.name
            target.resolve().relative_to(source.resolve())
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
            elif member.isfile():
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(archive.extractfile(member).read())
            else:
                raise ValueError("Unexpected non-file archive member")
    git(source, "init")
    for patch in LOCK["patches"]:
        git(source, "apply", "--check", str(ROOT / patch["file"]))
        git(source, "apply", str(ROOT / patch["file"]))
    for relative, expected in LOCK["overlays"].items():
        origin = ROOT / relative if relative.startswith(("src/", "tests/")) else ROOT / "overlays" / relative
        if sha256(origin) != expected:
            raise ValueError("Vitafy overlay checksum differs: " + relative)
        target = source / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(origin.read_bytes())
    verify(source)
    write_json(ROOT / "build/source-verification.json", dict(upstream=LOCK["upstream"],
               patch_sha256=LOCK["patches"], files=LOCK["prepared_file_sha256"]))
    print("Prepared and verified:", source)
    return source


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", type=Path, help="optional existing exact upstream checkout; never modified")
    args = parser.parse_args()
    prepare(args.upstream)
