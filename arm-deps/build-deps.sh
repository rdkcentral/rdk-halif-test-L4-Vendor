#!/bin/bash
# Builds every test dependency with build-dep.sh and packages the files it installed
# as arm-deps/sysroot/. build-arm.sh copies that tree into the SDK sysroot before the test build.
#
# Run inside the rdk-kirkstone container, as root (./build-arm-docker.sh deps does this).
# Env: HOST_UID / HOST_GID (optional) - owner for the packaged output, so the host user can read and delete it.
set -e

ARM_DEPS=$(cd "$(dirname "$0")" && pwd)
OUT=${ARM_DEPS}/sysroot
COMPONENTS="zlib libffi pcre glib orc libxml2 json-glib gstreamer gst-plugins-base gst-devtools ffmpeg curl libfyaml googletest"

# Same environment as build-dep.sh, so the preflight checks the tools the component builds will use.
. /opt/toolchains/rdk-glibc-x86_64-arm-toolchain/environment-setup-armv7vet2hf-neon-oe-linux-gnueabi
export PATH=/usr/local/bin:${PATH}
export M4=/bin/m4

MISSING=""
for tool in git curl patch tar xz bzip2 sha256sum make cmake ninja autoconf automake libtoolize msgfmt pkg-config qemu-arm; do
    command -v "${tool}" >/dev/null 2>&1 || MISSING="${MISSING} ${tool}"
done
[ -x /usr/local/bin/meson ] || MISSING="${MISSING} /usr/local/bin/meson"
[ -x "${M4}" ] || MISSING="${MISSING} ${M4}"
if [ -n "${MISSING}" ]; then
    echo "ERROR: host tools missing in the container:${MISSING}" >&2
    echo "Install them once as described in docs/04_README_Dependencies.md (section 2.2)." >&2
    exit 1
fi

# Files newer than this mark are the ones the component builds installed into the SDK sysroot.
MARK=$(mktemp)
trap 'rm -f "${MARK}"' EXIT

for component in ${COMPONENTS}; do
    echo "== building ${component}"
    bash "${ARM_DEPS}/build-dep.sh" "${component}"
done

rm -rf "${OUT}"
mkdir -p "${OUT}"
(cd "${SDKTARGETSYSROOT}" && find . -cnewer "${MARK}" \( -type f -o -type l \) -print0 | tar --null -T - -cf -) \
    | tar --no-same-owner -C "${OUT}" -xf -

if [ -n "${HOST_UID}" ] && [ -n "${HOST_GID}" ]; then
    chown -R "${HOST_UID}:${HOST_GID}" "${OUT}"
fi

echo "Packaged $(find "${OUT}" \( -type f -o -type l \) | wc -l) files into ${OUT}"
