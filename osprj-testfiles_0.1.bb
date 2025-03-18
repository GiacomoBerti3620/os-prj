SUMMARY = "Test files"
DESCRIPTION = "Copy test files"
LICENSE = "CLOSED"

SRC_URI += " \
    file://bar0_test.c \
    file://bar1_test.c \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

do_install:append() {
    install -d ${D}/home/root/test_files
    install -m 0777 ${UNPACKDIR}/bar0_test.c ${D}/home/root/test_files/bar0_test.c
    install -m 0777 ${UNPACKDIR}/bar1_test.c ${D}/home/root/test_files/bar1_test.c
}

FILES:${PN} += " \
    /home/root/test_files/bar0_test.c \
    /home/root/test_files/bar1_test.c \
"