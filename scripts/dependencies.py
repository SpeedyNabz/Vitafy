"""Download and hash-check the pinned Windows SDL development archives."""
from pathlib import PurePosixPath
import urllib.request
import zipfile
from common import ROOT, LOCK, sha256


def dependencies():
    downloads = ROOT / ".cache/downloads"
    downloads.mkdir(parents=True, exist_ok=True)
    destination = ROOT / ".cache/sdl"
    destination.mkdir(exist_ok=True)
    prefixes = []
    for item in LOCK["desktop_dependencies"]:
        archive = downloads / item["archive"]
        if not archive.exists():
            urllib.request.urlretrieve(item["url"], archive)
        if sha256(archive) != item["sha256"]:
            raise ValueError("Dependency checksum differs: " + item["name"])
        root = destination / item["root"]
        marker = root / ".verified-archive-sha256"
        if root.exists():
            if not marker.is_file() or marker.read_text().strip() != item["sha256"]:
                raise ValueError("Unverified dependency directory: " + str(root))
        else:
            with zipfile.ZipFile(archive) as bundle:
                for entry in bundle.infolist():
                    name = PurePosixPath(entry.filename)
                    if name.is_absolute() or ".." in name.parts or "\\" in entry.filename or ":" in entry.filename or not entry.filename.startswith(item["root"] + "/"):
                        raise ValueError("Unsafe dependency archive path")
                bundle.extractall(destination)
            marker.write_text(item["sha256"] + "\n")
        prefixes.append((root / "cmake").as_posix())
    return prefixes


if __name__ == "__main__":
    print("Verified SDL prefixes:", ";".join(dependencies()))
