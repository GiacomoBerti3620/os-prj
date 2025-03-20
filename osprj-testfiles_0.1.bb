SUMMARY = "Test files"
DESCRIPTION = "Copy test files"
LICENSE = "CLOSED"

SRC_URI += " \
    file://fft_test.c \
    file://base_test.c \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

do_install:append() {
    install -d ${D}/home/root/test_files
    install -m 0777 ${UNPACKDIR}/fft_test.c ${D}/home/root/test_files/fft_test.c
    install -m 0777 ${UNPACKDIR}/base_test.c ${D}/home/root/test_files/base_test.c
}

FILES:${PN} += " \
    /home/root/test_files/fft_test.c \
    /home/root/test_files/base_test.c \
"