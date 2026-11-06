#!/usr/bin/env python3
# Builds the standalone bundle that runs from /opt/l4-rdk-audio-video on a device that does not carry our
# dependency libraries: the stripped test binary, the shared libraries it needs from our dependency build,
# the device config with /opt paths, the test media and a launcher script.
# build-arm.sh runs this after packaging, with the RDK ARM SDK environment sourced: it provides
# SDKTARGETSYSROOT and the arm-oe-linux-gnueabi readelf and strip tools.
# Usage: python3 -I make-device-bundle.py <stage-dir> <output-dir>
import os
import shutil
import subprocess
import sys
import tarfile

SRC_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUNDLE = "l4-rdk-audio-video"
BINARY = "usr/bin/rdkgstreamerplugin/l4test/l4-rdk-audio-video"
DEVICE_VIDEO_DIR = "/usr/share/rdk/l4-rdk-audio-video/video-inputs"
BUNDLE_VIDEO_DIR = "/opt/l4-rdk-audio-video/share/video-inputs"
READELF = "arm-oe-linux-gnueabi-readelf"
STRIP = "arm-oe-linux-gnueabi-strip"
# Provided by the device's own C and C++ runtime; the bundle does not carry these.
DEVICE_BASE = {
    "ld-linux-armhf.so.3", "libc.so.6", "libm.so.6", "libdl.so.2", "libpthread.so.0",
    "librt.so.1", "libresolv.so.2", "libutil.so.1", "libgcc_s.so.1", "libstdc++.so.6",
}
RUN_SCRIPT = """#!/bin/sh
# Runs the L4 AV test from /opt/l4-rdk-audio-video with the bundled libraries.
# The GStreamer plugins are not bundled: GStreamer loads the device's own plugins from its system path.
# Usage: ./run.sh [UT-Core arguments]
BASE=/opt/l4-rdk-audio-video
export LD_LIBRARY_PATH="${BASE}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export L4_CONFIG="${BASE}/etc/platform.yaml"
export GST_REGISTRY="${GST_REGISTRY:-/tmp/l4-gst-registry.bin}"
export TMPDIR="${TMPDIR:-/tmp}"
cd "${BASE}" || exit 1
exec "${BASE}/bin/l4-rdk-audio-video" "$@"
"""


def needed(path):
    out = subprocess.run([READELF, "-d", path], capture_output=True, text=True, check=True).stdout
    return [line.split("[")[1].split("]")[0] for line in out.splitlines() if "(NEEDED)" in line]


def collect(binary, search):
    """Walks NEEDED entries from the binary. Returns ({soname: realpath}, [names not found])."""
    found = {}
    missing = []
    queue = [binary]
    while queue:
        for name in needed(queue.pop(0)):
            if name in found or name in missing:
                continue
            real = None
            for directory in search:
                candidate = os.path.join(directory, name)
                if os.path.exists(candidate):
                    real = os.path.realpath(candidate)
                    break
            if real is None:
                missing.append(name)
                continue
            found[name] = real
            queue.append(real)
    return found, missing


def copy_stripped(src, dst):
    shutil.copyfile(src, dst)
    os.chmod(dst, 0o755)
    subprocess.run([STRIP, "--strip-unneeded", dst], check=True)


def referenced_media(text):
    """Returns the video-inputs file names that the config refers to by device path."""
    names = []
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or ":" not in line:
            continue
        value = line.split(":", 1)[1].strip().strip('"')
        if value.startswith("file://"):
            value = value[len("file://"):]
        if value.startswith(DEVICE_VIDEO_DIR + "/"):
            names.append(os.path.basename(value))
    return names


def main(stage, out_root):
    binary = os.path.join(stage, BINARY)
    sysroot = os.environ.get("SDKTARGETSYSROOT")
    if not os.path.isfile(binary):
        sys.exit("ERROR: test binary not found, build it first: " + binary)
    if not sysroot:
        sys.exit("ERROR: SDKTARGETSYSROOT is not set; source the RDK ARM SDK environment first")

    found, missing = collect(binary, [os.path.join(sysroot, "lib"), os.path.join(sysroot, "usr/lib")])
    unresolved = [name for name in missing if name not in DEVICE_BASE]
    if unresolved:
        sys.exit("ERROR: shared libraries not found in the SDK sysroot: " + ", ".join(unresolved))

    video_dir = os.path.join(SRC_DIR, "video-inputs")
    with open(os.path.join(SRC_DIR, "config", "platform.yaml")) as f:
        original = f.read()
    missing_media = [name for name in referenced_media(original)
                     if not os.path.isfile(os.path.join(video_dir, name))]
    if missing_media:
        sys.exit("ERROR: platform.yaml refers to media missing from video-inputs/: " + ", ".join(missing_media))

    bundle = os.path.join(out_root, BUNDLE)
    archive = bundle + ".tar.gz"
    if os.path.exists(bundle):
        shutil.rmtree(bundle)
    for sub in ("bin", "lib", "etc", "share/video-inputs"):
        os.makedirs(os.path.join(bundle, sub))

    copy_stripped(binary, os.path.join(bundle, "bin", os.path.basename(binary)))

    bundled = sorted(name for name in found if name not in DEVICE_BASE)
    for name in bundled:
        copy_stripped(found[name], os.path.join(bundle, "lib", name))

    config = original.replace(DEVICE_VIDEO_DIR, BUNDLE_VIDEO_DIR)
    with open(os.path.join(bundle, "etc", "platform.yaml"), "w") as f:
        f.write(config)

    for name in sorted(os.listdir(video_dir)):
        shutil.copy2(os.path.join(video_dir, name), os.path.join(bundle, "share", "video-inputs", name))

    run_path = os.path.join(bundle, "run.sh")
    with open(run_path, "w") as f:
        f.write(RUN_SCRIPT)
    os.chmod(run_path, 0o755)

    lib_bytes = sum(os.path.getsize(os.path.join(bundle, "lib", name)) for name in bundled)
    with tarfile.open(archive, "w:gz") as tar:
        tar.add(bundle, arcname=BUNDLE)

    print("Device bundle: %s" % bundle)
    print("Device archive: %s" % archive)
    print("  %d bundled libraries, %d KB stripped" % (len(bundled), lib_bytes // 1024))
    print("  the device must provide: %s" % ", ".join(sorted(DEVICE_BASE)))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("Usage: make-device-bundle.py <stage-dir> <output-dir>")
    os.makedirs(sys.argv[2], exist_ok=True)
    main(sys.argv[1], sys.argv[2])
