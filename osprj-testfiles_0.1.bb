SUMMARY = "Test files"
DESCRIPTION = "Copy test files"
LICENSE = "CLOSED"

SRC_URI += " \
    file://fft_test.c \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

do_install:append() {
    install -d ${D}/home/root/test_files
    install -m 0777 ${UNPACKDIR}/fft_test.c ${D}/home/root/test_files/fft_test.c
}

FILES:${PN} += " \
    /home/root/test_files/fft_test.c \
"