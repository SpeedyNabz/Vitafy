"""Exercise real frontend navigation with offline mock data, never Spotify audio."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent


def main():
    out = ROOT / "artifacts/mock-preview"
    out.mkdir(parents=True, exist_ok=True)
    data = ROOT / "build/smoke-data"
    data.mkdir(exist_ok=True)
    script = out / "smoke.txt"
    script.write_text("""wait 800
shot home.bmp
state home.txt
click 34 150
wait 200
click 400 35
wait 150
text ambient
wait 100
key Return
wait 600
shot search.bmp
state search.txt
click 34 204
wait 600
shot library.bmp
state library.txt
click 34 368
wait 500
shot liked.bmp
state liked.txt
click 255 270
wait 700
key n
wait 400
shot now-playing.bmp
state now-playing.txt
key n
wait 400
click 34 420
wait 600
shot queue.bmp
state queue.txt
click 34 258
wait 400
shot settings.bmp
state settings.txt
background
capture before.bmp
wait 500
capture during.bmp
foreground
wait 300
shot after.bmp
quitshot forbidden.bmp
""")
    executable = ROOT / ("build/desktop/Release/VitafyPreview.exe" if os.name == "nt" else "build/desktop/VitafyPreview")
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_RENDER_DRIVER="software",
               VAM_MOCK_INSTANT="1", VAM_DATA_DIR=str(data) + os.sep, VAM_SCRIPT=str(script))
    for key in ("VAM_MOCK_FREE", "VAM_MOCK_STAY_OUT"):
        env.pop(key, None)
    forbidden = out / "forbidden.bmp"
    if forbidden.exists(): forbidden.unlink()
    with (out / "stderr.log").open("w") as log:
        subprocess.run([str(executable)], cwd=out, env=env, stdout=log, stderr=log, timeout=30, check=True,
                       creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    def state(name):
        return dict(line.split("=", 1) for line in (out / (name + ".txt")).read_text().splitlines())
    assert state("home")["tab"] == "0"
    assert state("search")["tab"] == "1" and state("search")["query"] == "ambient"
    assert state("library")["tab"] == "2"
    assert state("liked")["uri"].endswith(":collection") and state("liked")["depth"] == "2"
    assert state("now-playing")["np"] == "1"
    assert state("queue")["uri"] == "vam:queue"
    assert state("settings")["tab"] == "3"
    assert (out / "before.bmp").read_bytes() == (out / "during.bmp").read_bytes()
    assert (out / "after.bmp").read_bytes() != (out / "before.bmp").read_bytes()
    assert not forbidden.exists()
    for name in ("home", "search", "library", "liked", "now-playing", "queue", "settings"):
        with Image.open(out / (name + ".bmp")) as image:
            assert image.size == (960, 544)
            image.save(out / (name + ".png"))
    record = dict(status="passed", scope="offline mock UI; no Spotify login or audio",
                  assertions=["search entry and results", "library navigation", "account-derived Liked Songs URI",
                              "Now Playing", "queue", "overlay freezes drawing", "foreground resumes drawing", "quit draws no extra frame"],
                  executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                  source_sha256={p.relative_to(ROOT / "build/source").as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in (ROOT / "build/source").rglob("*") if p.is_file() and ".git" not in p.parts})
    (out / "verification.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps({k: v for k, v in record.items() if k != "source_sha256"}, indent=2))


if __name__ == "__main__":
    main()
