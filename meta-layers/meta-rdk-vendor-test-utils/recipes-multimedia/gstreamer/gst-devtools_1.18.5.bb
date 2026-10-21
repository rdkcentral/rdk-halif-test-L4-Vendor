SUMMARY = "GStreamer validation tool"
DESCRIPTION = "Tools and libraries for validating GStreamer pipelines"
HOMEPAGE = "https://gstreamer.freedesktop.org/documentation/gst-devtools/"
SECTION = "multimedia"

LICENSE = "LGPL-2.1-or-later"
LIC_FILES_CHKSUM = "file://validate/COPYING;md5=a6f89e2100d9b6cdffcea4f398e37343"

SRC_URI = "https://gstreamer.freedesktop.org/src/gst-devtools/gst-devtools-${PV}.tar.xz"
SRC_URI[sha256sum] = "fecffc86447daf5c2a06843c757a991d745caa2069446a0d746e99b13f7cb079"

DEPENDS = "json-glib glib-2.0 glib-2.0-native gstreamer1.0 gstreamer1.0-plugins-base"
RRECOMMENDS:${PN} = "git"

FILES:${PN} += "${datadir}/gstreamer-1.0/* ${libdir}/gst-validate-launcher/* ${libdir}/gstreamer-1.0/*"

inherit meson pkgconfig gettext upstream-version-is-even gobject-introspection

EXTRA_OEMESON += " \
    -Ddoc=disabled \
    -Ddebug_viewer=disabled \
    -Dtests=disabled \
    -Dvalidate=enabled \
"

do_install:append() {
    for file_name in ${bindir}/gst-validate-launcher \
        ${libdir}/gst-validate-launcher/python/launcher/config.py; do
        if [ -e "${D}${file_name}" ]; then
            sed -i -e 's,${B},/usr/src/debug/${PN},g' \
                -e 's,${S},/usr/src/debug/${PN},g' "${D}${file_name}"
        fi
    done
}

GIR_MESON_ENABLE_FLAG = "enabled"
GIR_MESON_DISABLE_FLAG = "disabled"