#!/usr/bin/env bash
# Cross-compile l4-rdk-audio-video for the 32-bit ARM target (armv7vet2hf) with the RDK toolchain.
# Follows the "For ARM target" steps of rdk-halif-aidl-unified-vts: source the RDK ARM SDK first,
# then build. Run it inside the rdk-kirkstone Docker image, where the SDK lives under /opt/toolchains.
#
# Usage: ./build-arm.sh          build the test and the device bundle in build-arm/device/
#        ./build-arm.sh clean    remove build-arm/
set -e

SRC_DIR=$(cd "$(dirname "$0")" && pwd)
WORK_DIR=${WORK_DIR:-${SRC_DIR}/build-arm}

if [ "$1" = "clean" ]; then
    rm -rf "${WORK_DIR}"
    exit 0
fi

RDK_SDK_ENV=${RDK_SDK_ENV:-/opt/toolchains/rdk-glibc-x86_64-arm-toolchain/environment-setup-armv7vet2hf-neon-oe-linux-gnueabi}
UT_CORE_VERSION=${UT_CORE_VERSION:-5.1.0}
UT_CONTROL_VERSION=${UT_CONTROL_VERSION:-2.1.0}

UT_CORE_DIR=${WORK_DIR}/ut-core
UT_CONTROL_DIR=${WORK_DIR}/ut-control
BUILD_DIR=${WORK_DIR}/cmake
STAGE_DIR=${WORK_DIR}/stage

# The SDK script sets CC, CXX, CFLAGS, LDFLAGS and PKG_CONFIG_SYSROOT_DIR for the ARM target.
if [ ! -f "${RDK_SDK_ENV}" ]; then
    echo "ERROR: RDK ARM SDK environment not found: ${RDK_SDK_ENV}" >&2
    exit 1
fi
. "${RDK_SDK_ENV}"
if [ -z "${CC}" ]; then
    echo "ERROR: CC is not set after sourcing ${RDK_SDK_ENV}" >&2
    exit 1
fi

# Lists the test dependencies the SDK sysroot is missing.
missing_deps() {
    local out="" mod
    for mod in gstreamer-1.0 gstreamer-audio-1.0 gstreamer-app-1.0 gst-validate-1.0 libavformat libavcodec libavutil libcurl libfyaml; do
        pkg-config --exists "${mod}" || out="${out} ${mod}"
    done
    printf '#include <gtest/gtest.h>\n#include <gmock/gmock.h>\n' | ${CXX} -E -x c++ - >/dev/null 2>&1 \
        || out="${out} gtest/gmock"
    echo "${out}"
}

# Prebuilt test dependencies (a sysroot tree built by arm-deps/build-dep.sh) are copied into the SDK sysroot when it lacks them.
DEP_SYSROOT=${DEP_SYSROOT:-${SRC_DIR}/arm-deps/sysroot}
if [ -n "$(missing_deps)" ] && [ -d "${DEP_SYSROOT}" ]; then
    if [ -z "${SDKTARGETSYSROOT:-}" ]; then
        echo "ERROR: SDKTARGETSYSROOT is not set after sourcing ${RDK_SDK_ENV}" >&2
        exit 1
    fi
    cp -a "${DEP_SYSROOT}/." "${SDKTARGETSYSROOT}/"
fi

# Fail early and list everything the SDK sysroot is missing.
MISSING=$(missing_deps)
if [ -n "${MISSING}" ]; then
    echo "ERROR: missing in the SDK sysroot:${MISSING}" >&2
    echo "Build them once with: ./build-arm-docker.sh deps (writes ${DEP_SYSROOT})." >&2
    echo "Or point DEP_SYSROOT at a prebuilt sysroot tree." >&2
    exit 1
fi

if [ ! -d "${SRC_DIR}/video-inputs" ]; then
    echo "ERROR: ${SRC_DIR}/video-inputs not found. Copy the test media folder into the project root." >&2
    exit 1
fi

# Clone once, then check out the exact release tag.
fetch_tag() {
    local repo=$1 tag=$2 dir=$3
    if [ ! -d "${dir}/.git" ]; then
        git clone -q "https://github.com/rdkcentral/${repo}.git" "${dir}"
    fi
    git -C "${dir}" fetch -q --tags origin
    git -C "${dir}" checkout -q "refs/tags/${tag}"
}
fetch_tag ut-core "${UT_CORE_VERSION}" "${UT_CORE_DIR}"
fetch_tag ut-control "${UT_CONTROL_VERSION}" "${UT_CONTROL_DIR}"

# The install tree uses the paths the binary and config expect on a device: the binary under
# /usr/bin/rdkgstreamerplugin/l4test, platform.yaml under /etc/rdk/ut-core, and video-inputs under
# /usr/share/rdk/l4-rdk-audio-video. The device bundle is built from this tree.
# The compiled-in default config points at the bundle so the binary works without L4_CONFIG.
cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_SYSCONFDIR=/etc \
    -DUT_CORE_DIR="${UT_CORE_DIR}" \
    -DUT_CONTROL_DIR="${UT_CONTROL_DIR}" \
    -DL4_CONFIG_INSTALL_PATH=/opt/l4-rdk-audio-video/etc/platform.yaml
cmake --build "${BUILD_DIR}" -j"$(nproc)"

rm -rf "${STAGE_DIR}"
DESTDIR="${STAGE_DIR}" cmake --install "${BUILD_DIR}"

# Refuse to package a host binary: the output must be an ARM ELF.
BIN=${STAGE_DIR}/usr/bin/rdkgstreamerplugin/l4test/l4-rdk-audio-video
if ! "${READELF:-readelf}" -h "${BIN}" | grep -q 'Machine:.*ARM'; then
    echo "ERROR: ${BIN} is not an ARM binary" >&2
    exit 1
fi

# Device bundle: the test binary, the libraries it needs from our dependency build, config, media, and run.sh.
# It runs from /opt/l4-rdk-audio-video (see docs/02_README_Docker_Run.md).
python3 -I "${SRC_DIR}/arm-deps/make-device-bundle.py" "${STAGE_DIR}" "${WORK_DIR}/device"
