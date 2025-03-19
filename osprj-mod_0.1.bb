SUMMARY = "External Linux kernel module for OS project"
DESCRIPTION = "${SUMMARY}"
LICENSE = "CLOSED"

inherit module

SRC_URI = "file://Makefile \
           file://virt_fft_acc_drv.c \
          "

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.

RPROVIDES:${PN} += "kernel-module-osprj"
