"""Build/test the standalone desktop mock or package the real native backend."""
import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import zipfile
from common import ROOT, LOCK, run, sha256, write_json
from prepare import prepare


def sfo(data):
    magic, _, keys, values, count = struct.unpack_from("<4sIIII", data)
    if magic != b"\0PSF": raise ValueError("Invalid SFO")
    result = {}
    for i in range(count):
        key_off, kind, size, _, value_off = struct.unpack_from("<HHIII", data, 20 + 16*i)
        key = data[keys+key_off:data.index(b"\0", keys+key_off)].decode()
        raw = data[values+value_off:values+value_off+size]
        result[key] = struct.unpack("<I", raw)[0] if kind == 0x404 else raw.rstrip(b"\0").decode()
    return result


def desktop(source):
    build = ROOT / "build/desktop"
    command = ["cmake", "-S", ROOT / "cmake/desktop", "-B", build]
    if os.name == "nt":
        from dependencies import dependencies
        command += ["-G", "Visual Studio 17 2022", "-A", "x64", "-DCMAKE_PREFIX_PATH=" + ";".join(dependencies())]
    else:
        command += ["-DCMAKE_BUILD_TYPE=Release"]
    run(command)
    run(["cmake", "--build", build, "--config", "Release", "--parallel", "4"])
    run(["ctest", "--test-dir", build, "-C", "Release", "--output-on-failure"])
    run([sys.executable, ROOT / "scripts/test_preview.py"])
    run([sys.executable, ROOT / "scripts/test_library_sync.py"])
    run([sys.executable, ROOT / "scripts/test_library_controls.py"])
    run([sys.executable, ROOT / "scripts/test_search_keyboard.py"])
    run([sys.executable, ROOT / "scripts/test_large_playback.py"])


def native(source, args):
    sdk_arg = args.sdk or os.environ.get("VITASDK")
    if not sdk_arg: raise ValueError("Set VITASDK or supply --sdk")
    sdk = Path(sdk_arg).resolve()
    env = dict(os.environ, VITASDK=sdk.as_posix())
    env["PATH"] = str(sdk / "bin") + os.pathsep + env["PATH"]
    env["CC_armv7_sony_vita_newlibeabihf"] = str(sdk / "bin/arm-vita-eabi-gcc")
    env["AR_armv7_sony_vita_newlibeabihf"] = str(sdk / "bin/arm-vita-eabi-ar")
    backend = args.backend_dir.resolve() if args.backend_dir else source / "backend/librespot-vam"
    target = backend / "target/armv7-sony-vita-newlibeabihf"
    library = target / "release/libvam_backend.a"
    if args.reuse_backend:
        if LOCK.get("backend_api_version", 3) != 3:
            raise ValueError("This source requires a fresh ABI v6 backend; the historical archive is ABI v3")
        if sha256(library) != LOCK["historical_backend_sha256"]:
            raise ValueError("Reused backend differs from the recorded verified archive")
    else:
        if args.backend_dir: raise ValueError("--backend-dir requires --reuse-backend; external sources are never rebuilt")
        result=run(["cargo", "+" + LOCK["sdk"]["rust_toolchain"], "build", "--locked", "--release", "--lib",
             "--target", "armv7-sony-vita-newlibeabihf", "-Z", "build-std=std,panic_abort",
             "--message-format=json-render-diagnostics"], cwd=backend, env=env, stdout=subprocess.PIPE, text=True)
        (ROOT/"build/backend-cargo-messages.jsonl").write_text(result.stdout)
        ring_outputs=[]
        for line in result.stdout.splitlines():
            try: message=json.loads(line)
            except ValueError: continue
            package=message.get("package_id","")
            if message.get("reason")=="build-script-executed" and ("/ring#" in package or "ring@" in package):
                out=Path(message["out_dir"])
                if "armv7-sony-vita-newlibeabihf" in out.parts: ring_outputs.append(out)
    # Cargo identifies the active build-script output, including when multiple
    # debug/profile variants coexist. Never guess using directory timestamps.
    rings = sorted(target.rglob("libring_core_0_17_14_.a")) if args.reuse_backend else sorted({p for out in ring_outputs for p in out.rglob("libring_core_0_17_14_.a")})
    if len(rings) != 1: raise ValueError("Expected exactly one ring assembly archive; use a clean backend build")
    build = ROOT / ("build/native-home" if args.home_playback else "build/native")
    support = ROOT / "build/native-support"
    support.mkdir(parents=True, exist_ok=True)
    suffix = ".exe" if os.name == "nt" else ""
    run([sdk / ("bin/arm-vita-eabi-gcc" + suffix), "-O2", "-g", "-c", source / "backend/librespot-vam/vita-shim/vita_shim.c", "-o", support / "vita_shim.o"], env=env)
    shim = support / "libvam_vita_shim.a"
    run([sdk / ("bin/arm-vita-eabi-ar" + suffix), "rcs", shim, support / "vita_shim.o"], env=env)
    libs = ";".join(["-Wl,--wrap=pthread_cond_timedwait", rings[0].as_posix(), shim.as_posix(), "SceNet_stub", "SceNetCtl_stub", "pthread"])
    run(["cmake", "-S", source, "-B", build, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
         "-DCMAKE_TOOLCHAIN_FILE=" + (sdk / "share/vita.toolchain.cmake").as_posix(), "-DVAM_BACKEND=external",
         "-DVAM_BACKEND_LIB=" + library.as_posix(), "-DVAM_BACKEND_LINK_LIBS=" + libs,
         "-DVITAFY_VERSION=" + LOCK["version"],
         "-DVITAFY_HOME_PLAYBACK=" + ("ON" if args.home_playback else "OFF")], env=env)
    run(["cmake", "--build", build, "--parallel", "4"], env=env)
    package = build / "Vitafy.vpk"
    with zipfile.ZipFile(package) as archive:
        if archive.testzip(): raise ValueError("VPK CRC failure")
        metadata = sfo(archive.read("sce_sys/param.sfo"))
        for key, expected in dict(TITLE="Vitafy", TITLE_ID="VTAM00001", APP_VER="01.31" if args.home_playback else "01.32", ATTRIBUTE=32776 if args.home_playback else 32768).items():
            if metadata.get(key) != expected: raise ValueError("Wrong package metadata: " + key)
        if archive.read("eboot.bin")[:4] != b"SCE\0": raise ValueError("Invalid SELF")
        if len(set(archive.namelist())) != len(archive.namelist()): raise ValueError("Duplicate archive entries")
        from PIL import Image
        from io import BytesIO
        for name, size in [("sce_sys/icon0.png", (128,128)), ("sce_sys/livearea/contents/bg.png", (840,500)), ("sce_sys/livearea/contents/startup.png", (280,158))]:
            if archive.read(name) != (source / name).read_bytes(): raise ValueError("Stale artwork")
            with Image.open(BytesIO(archive.read(name))) as art:
                if art.mode != "P" or art.size != size: raise ValueError("Invalid Vita artwork format")
        for name in archive.namelist():
            if any(word in name.lower() for word in ("credential", "private/", ".env", ".git/")) or name.endswith(".log"):
                raise ValueError("Private file in VPK")
    symbols = subprocess.check_output([str(sdk / ("bin/arm-vita-eabi-nm" + suffix)), str(build / "Vitafy")], text=True, env=env)
    names = {line.split()[-1] for line in symbols.splitlines() if line.split()}
    required = {"sceNotificationUtilSendNotification", "notifications_update", "platform_poll_lifecycle", "sceAudioOutOpenPort", "sceKernelPowerTick", "sceKernelGetFreeMemorySize", "vam_init", "vam_shutdown", "vam_play_ordered", "vam_playlist_edit", "vam_playlist_entry_edit", "vam_queue_edit", "vam_order_begin", "vam_order_append", "vam_order_commit", "vam_clear_metadata_cache"}
    if not required <= names: raise ValueError("Missing native imports/functions")
    destination = ROOT / "artifacts" / ("Vitafy-" + LOCK["version"] + ("-home-experimental" if args.home_playback else ""))
    destination.mkdir(parents=True, exist_ok=True)
    shutil.copy2(package, destination / package.name)
    record = dict(scope="standalone native package; no device test", sdk=LOCK["sdk"], upstream=LOCK["upstream"],
                  source_lock_sha256=sha256(ROOT / "source-lock.json"), backend_sha256=sha256(library),
                  backend_build="reused verified archive" if args.reuse_backend else "fresh Cargo build",
                  vpk_sha256=sha256(package), elf_sha256=sha256(build / "Vitafy"), velf_sha256=sha256(build / "Vitafy.velf"),
                  package=metadata, native_imports="verified", hardware="pending",
                  backend_api_version=LOCK.get("backend_api_version",3),
                  quality_policy="Best: lossless first; Medium: <=160 kbps; Low: <=96 kbps; Vita PCM S16",
                  debug_information="frontend ELF/VELF and fresh Rust backend retained",
                  lifecycle_profile="Home experiment; game forced-close unsafe" if args.home_playback else "foreground")
    write_json(destination / "verification.json", record)
    (destination / "SHA256SUMS.txt").write_text(record["vpk_sha256"] + "  Vitafy.vpk\n")
    print("Verified native package:", destination / "Vitafy.vpk")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("desktop", "vita"))
    parser.add_argument("--upstream", type=Path)
    parser.add_argument("--sdk")
    parser.add_argument("--home-playback", action="store_true", help="separate experimental Home package; game forced-close remains unsafe")
    parser.add_argument("--reuse-backend", action="store_true", help="use only the hash-pinned historical archive")
    parser.add_argument("--backend-dir", type=Path, help="optional existing backend; read-only and hash checked")
    options = parser.parse_args()
    source = prepare(options.upstream)
    desktop(source) if options.mode == "desktop" else native(source, options)
