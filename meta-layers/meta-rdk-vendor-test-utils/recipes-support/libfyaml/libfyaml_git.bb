SUMMARY = "YAML parser and emitter library"
DESCRIPTION = "Fully featured YAML parser and emitter used by UT control"
HOMEPAGE = "https://github.com/pantoniou/libfyaml"
SECTION = "libs"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=6399094fbc639a289cfca2d660c010aa"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI = "git://github.com/pantoniou/libfyaml.git;protocol=https;branch=master \
           file://0001-allocator-durable-skip-pointer-alignment-on-32bit.patch \
"
SRCREV = "1a850a82b3b4ce1e21e4dedb15c7fa677ba2a6c5"

S = "${WORKDIR}/git"

inherit autotools pkgconfig

EXTRA_OECONF = " \
    --disable-static \
    --disable-tests \
"

FILES:${PN} += "${libdir}/libfyaml.so.*"
FILES:${PN}-dev += "${libdir}/libfyaml.so ${libdir}/pkgconfig/libfyaml.pc"