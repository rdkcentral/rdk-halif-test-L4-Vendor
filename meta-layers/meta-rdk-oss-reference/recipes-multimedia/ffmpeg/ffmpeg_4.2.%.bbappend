FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " file://CVE-2020-12284_fix.patch \
                   file://CVE-2021-3566_fix.patch \
                   file://CVE-2021-38291_fix.patch \
                   file://CVE-2022-1475_fix.patch \
                   file://CVE-2022-3109_fix.patch \
                   file://CVE-2022-3341_fix.patch \
                 "
