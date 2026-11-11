#!/bin/bash
# Cross-build one test dependency for the armv7vet2hf-neon RDK SDK and install it into that SDK's sysroot.
# Run in the RDK ARM SDK environment (inside the rdk-kirkstone container). Usage: build-dep.sh <component>
# Installs into SDKTARGETSYSROOT. The resulting files are packaged as the prebuilt sysroot in arm-deps/sysroot.
# The SDK environment supplies CC/CXX, PKG_CONFIG_* and CONFIG_SITE (cached answers for configure checks
# that would otherwise need to run target code).

set -e
. /opt/toolchains/rdk-glibc-x86_64-arm-toolchain/environment-setup-armv7vet2hf-neon-oe-linux-gnueabi

# The SDK puts its own meson (0.61.3, a wrapper script) first on PATH. Use the pinned pip meson instead.
export PATH=/usr/local/bin:${PATH}
# The SDK sets M4=m4 by name. bison runs m4 with GNU options (--gnu), so pin the system GNU m4.
export M4=/bin/m4

set -e

COMPONENT=$1
WORK=/tmp/arm-deps/${COMPONENT}
LOG=/tmp/arm-deps/${COMPONENT}.log
PATCHES="$(cd "$(dirname "$0")" && pwd)/patches"
JOBS=$(nproc)
mkdir -p "${WORK}"
: > "${LOG}"

# Run one step with its output kept in the log. On failure, show the end of the log and fail the build.
run() {
    if ! "$@" >> "${LOG}" 2>&1; then
        echo "FAILED (${COMPONENT}): $*" >&2
        tail -150 "${LOG}" >&2
        exit 1
    fi
}

# Download a source archive into the work directory. Retries ride out transient network failures, which
# otherwise stop the whole build at the first download that times out.
fetch() {
    local try
    for try in 1 2 3 4 5; do
        if curl -sSfL -o "${WORK}/$2" "$1" >> "${LOG}" 2>&1; then
            return 0
        fi
        echo "download attempt ${try} failed (${COMPONENT}): $1" >> "${LOG}"
        sleep 10
    done
    echo "FAILED (${COMPONENT}): could not download $1" >&2
    tail -150 "${LOG}" >&2
    exit 1
}

# Configure, build and install an autotools package. Installs with DESTDIR so the .pc files keep /usr
# prefixes; PKG_CONFIG_SYSROOT_DIR adds the sysroot back when other packages are configured.
autotools() {
    cd "$1"
    shift
    run ./configure --build=x86_64-linux --host=arm-oe-linux-gnueabi --prefix=/usr "$@"
    run make -j"${JOBS}"
    run make install DESTDIR="${SDKTARGETSYSROOT}"
}

# Write the Meson cross file for the SDK compiler. Test binaries run under qemu-arm, with the sysroot as the
# library root, so configure checks that execute code still work.
write_meson_cross() {
    cat > "${WORK}/meson-cross.txt" <<EOF
[binaries]
c = ['arm-oe-linux-gnueabi-gcc', '-march=armv7ve', '-mthumb', '-mfpu=neon', '-mfloat-abi=hard', '--sysroot=${SDKTARGETSYSROOT}']
cpp = ['arm-oe-linux-gnueabi-g++', '-march=armv7ve', '-mthumb', '-mfpu=neon', '-mfloat-abi=hard', '--sysroot=${SDKTARGETSYSROOT}']
ar = 'arm-oe-linux-gnueabi-ar'
strip = 'arm-oe-linux-gnueabi-strip'
ld = 'arm-oe-linux-gnueabi-ld'
objcopy = 'arm-oe-linux-gnueabi-objcopy'
pkgconfig = 'pkg-config'
exe_wrapper = ['qemu-arm', '-L', '${SDKTARGETSYSROOT}']

[properties]
sys_root = '${SDKTARGETSYSROOT}'

[host_machine]
system = 'linux'
cpu_family = 'arm'
cpu = 'armv7'
endian = 'little'
EOF
}

# Configure, build and install a Meson package. Extra arguments are Meson options.
meson_build() {
    local src=$1
    shift
    write_meson_cross
    # Meson 0.61 takes the source directory from the current directory, so set up from inside the source tree.
    cd "${src}"
    run meson setup "${WORK}/build" --cross-file "${WORK}/meson-cross.txt" \
        --prefix=/usr --libdir=lib --buildtype=release "$@"
    run meson compile -C "${WORK}/build" -j "${JOBS}"
    run env DESTDIR="${SDKTARGETSYSROOT}" meson install -C "${WORK}/build"
}

build_zlib() {
    fetch https://github.com/madler/zlib/archive/refs/tags/v1.2.11.tar.gz zlib.tar.gz
    run tar -C "${WORK}" -xf "${WORK}/zlib.tar.gz"
    cd "${WORK}/zlib-1.2.11"
    run env CHOST=arm-oe-linux-gnueabi ./configure --prefix=/usr --shared
    run make -j"${JOBS}"
    run make install DESTDIR="${SDKTARGETSYSROOT}"
}

build_libffi() {
    fetch https://github.com/libffi/libffi/releases/download/v3.3/libffi-3.3.tar.gz libffi.tar.gz
    run tar -C "${WORK}" -xf "${WORK}/libffi.tar.gz"
    autotools "${WORK}/libffi-3.3" --disable-static --disable-docs
}

build_pcre() {
    # PCRE 8.x, the release GLib 2.66 links against (libpcre). Taken from the Ubuntu archive.
    fetch https://archive.ubuntu.com/ubuntu/pool/main/p/pcre3/pcre3_8.39.orig.tar.bz2 pcre.tar.bz2
    run tar -C "${WORK}" -xf "${WORK}/pcre.tar.bz2"
    autotools "${WORK}/pcre-8.39" --enable-utf8 --enable-unicode-properties --disable-cpp --disable-static
}

build_glib() {
    fetch https://download.gnome.org/sources/glib/2.66/glib-2.66.8.tar.xz glib.tar.xz
    run tar -C "${WORK}" -xf "${WORK}/glib.tar.xz"
    meson_build "${WORK}/glib-2.66.8" -Dman=false -Dgtk_doc=false -Dnls=disabled -Dselinux=disabled \
        -Dlibmount=disabled -Dxattr=false -Dsysprof=disabled -Dinstalled_tests=false
}

build_orc() {
    fetch https://gstreamer.freedesktop.org/src/orc/orc-0.4.32.tar.xz orc.tar.xz
    run tar -C "${WORK}" -xf "${WORK}/orc.tar.xz"
    meson_build "${WORK}/orc-0.4.32" -Dbenchmarks=disabled -Dexamples=disabled -Dgtk_doc=disabled \
        -Dtests=disabled -Dtools=disabled -Dorc-test=disabled
}

build_libxml2() {
    fetch https://download.gnome.org/sources/libxml2/2.9/libxml2-2.9.10.tar.xz libxml2.tar.xz
    run tar -C "${WORK}" -xf "${WORK}/libxml2.tar.xz"
    autotools "${WORK}/libxml2-2.9.10" --without-python --without-lzma --without-icu --disable-static
}

build_json_glib() {
    fetch https://download.gnome.org/sources/json-glib/1.6/json-glib-1.6.6.tar.xz json-glib.tar.xz
    run tar -C "${WORK}" -xf "${WORK}/json-glib.tar.xz"
    meson_build "${WORK}/json-glib-1.6.6" -Dintrospection=disabled -Dgtk_doc=disabled \
        -Dtests=false -Dman=false
}

build_gstreamer() {
    fetch https://gstreamer.freedesktop.org/src/gstreamer/gstreamer-1.18.5.tar.xz gstreamer.tar.xz
    run tar -C "${WORK}" -xf "${WORK}/gstreamer.tar.xz"
    # check=enabled builds gstreamer-check-1.0 (its libcheck copy is bundled), which gst-validate requires.
    meson_build "${WORK}/gstreamer-1.18.5" -Dtests=disabled -Dexamples=disabled -Ddoc=disabled \
        -Dintrospection=disabled -Dbenchmarks=disabled -Dcheck=enabled -Dtools=disabled -Dgtk_doc=disabled
}

build_gst_plugins_base() {
    fetch https://gstreamer.freedesktop.org/src/gst-plugins-base/gst-plugins-base-1.18.5.tar.xz gst-plugins-base.tar.xz
    run tar -C "${WORK}" -xf "${WORK}/gst-plugins-base.tar.xz"
    meson_build "${WORK}/gst-plugins-base-1.18.5" -Dauto_features=disabled -Dtests=disabled \
        -Dexamples=disabled -Ddoc=disabled -Dintrospection=disabled -Dtools=disabled -Dnls=disabled
}

build_gst_devtools() {
    fetch https://gstreamer.freedesktop.org/src/gst-devtools/gst-devtools-1.18.5.tar.xz gst-devtools.tar.xz
    # Same checksum as the gst-devtools_1.18.5 recipe in meta-layers.
    run sh -c "echo 'fecffc86447daf5c2a06843c757a991d745caa2069446a0d746e99b13f7cb079  ${WORK}/gst-devtools.tar.xz' | sha256sum -c -"
    run tar -C "${WORK}" -xf "${WORK}/gst-devtools.tar.xz"
    meson_build "${WORK}/gst-devtools-1.18.5" -Ddoc=disabled -Ddebug_viewer=disabled -Dtests=disabled \
        -Dvalidate=enabled -Dintrospection=disabled
}

build_ffmpeg() {
    # The 4.2 series that ffmpeg_4.2.%.bbappend targets. The bbappend's CVE patches are not in the checkout,
    # so this is the upstream 4.2.10 release tag.
    fetch https://github.com/FFmpeg/FFmpeg/archive/refs/tags/n4.2.10.tar.gz ffmpeg.tar.gz
    run tar -C "${WORK}" -xf "${WORK}/ffmpeg.tar.gz"
    cd "${WORK}/FFmpeg-n4.2.10"
    run ./configure --enable-cross-compile --cross-prefix=arm-oe-linux-gnueabi- --arch=armv7 \
        --target-os=linux --sysroot="${SDKTARGETSYSROOT}" --prefix=/usr \
        --enable-shared --disable-static --disable-programs --disable-doc --disable-debug --enable-pic \
        --extra-cflags="-march=armv7ve -mthumb -mfpu=neon -mfloat-abi=hard" \
        --extra-ldflags="-march=armv7ve -mthumb -mfpu=neon -mfloat-abi=hard" \
        --target-exec="qemu-arm -L ${SDKTARGETSYSROOT}"
    run make -j"${JOBS}"
    run make install DESTDIR="${SDKTARGETSYSROOT}"
}

build_curl() {
    # No TLS: the test code only links libcurl (see CMakeLists.txt), it does not open https URLs.
    fetch https://curl.se/download/curl-7.88.1.tar.xz curl.tar.xz
    run tar -C "${WORK}" -xf "${WORK}/curl.tar.xz"
    autotools "${WORK}/curl-7.88.1" --without-ssl --disable-ldap --disable-ldaps --without-libidn2 \
        --without-brotli --without-zstd --without-nghttp2 --without-libpsl --disable-manual --disable-static
}

build_libfyaml() {
    # Pinned to the libfyaml_git.bb SRCREV, with the same 32-bit allocator patch.
    run rm -rf "${WORK}/libfyaml"
    run git clone --quiet https://github.com/pantoniou/libfyaml.git "${WORK}/libfyaml"
    cd "${WORK}/libfyaml"
    run git checkout --quiet 1a850a82b3b4ce1e21e4dedb15c7fa677ba2a6c5
    run sh -c "patch -p1 < ${PATCHES}/0001-allocator-durable-skip-pointer-alignment-on-32bit.patch"
    run ./bootstrap.sh
    autotools "${WORK}/libfyaml" --disable-static --disable-tests
}

build_googletest() {
    # Static and position-independent, so the test binary does not need libgtest.so on the STB.
    fetch https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz googletest.tar.gz
    run tar -C "${WORK}" -xf "${WORK}/googletest.tar.gz"
    cat > "${WORK}/toolchain.cmake" <<EOF
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-oe-linux-gnueabi-gcc)
set(CMAKE_CXX_COMPILER arm-oe-linux-gnueabi-g++)
set(CMAKE_SYSROOT ${SDKTARGETSYSROOT})
set(CMAKE_FIND_ROOT_PATH ${SDKTARGETSYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_C_FLAGS_INIT "-march=armv7ve -mthumb -mfpu=neon -mfloat-abi=hard -fPIC")
set(CMAKE_CXX_FLAGS_INIT "-march=armv7ve -mthumb -mfpu=neon -mfloat-abi=hard -fPIC")
EOF
    run cmake -S "${WORK}/googletest-release-1.11.0" -B "${WORK}/build" \
        -DCMAKE_TOOLCHAIN_FILE="${WORK}/toolchain.cmake" -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DBUILD_GMOCK=ON -DINSTALL_GTEST=ON -Dgtest_build_tests=OFF -Dgtest_build_samples=OFF
    run cmake --build "${WORK}/build" -j"${JOBS}"
    run env DESTDIR="${SDKTARGETSYSROOT}" cmake --install "${WORK}/build"
}

case "${COMPONENT}" in
    zlib) build_zlib ;;
    libffi) build_libffi ;;
    pcre) build_pcre ;;
    glib) build_glib ;;
    orc) build_orc ;;
    libxml2) build_libxml2 ;;
    json-glib) build_json_glib ;;
    gstreamer) build_gstreamer ;;
    gst-plugins-base) build_gst_plugins_base ;;
    gst-devtools) build_gst_devtools ;;
    ffmpeg) build_ffmpeg ;;
    curl) build_curl ;;
    libfyaml) build_libfyaml ;;
    googletest) build_googletest ;;
    *)
        echo "unknown component: ${COMPONENT}" >&2
        exit 2
        ;;
esac

rm -rf "${WORK}" "${LOG}"
echo "installed ${COMPONENT} into ${SDKTARGETSYSROOT}"
