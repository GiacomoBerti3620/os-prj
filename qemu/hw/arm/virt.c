


// >>>>> ROBA <<<<<


/* Addresses and sizes of our components.
 * 0..128MB is space for a flash device so we can run bootrom code such as UEFI.
 * 128MB..256MB is used for miscellaneous device I/O.
 * 256MB..1GB is reserved for possible future PCI support (ie where the
 * PCI memory window will go if we add a PCI host controller).
 * 1GB and up is RAM (which may happily spill over into the
 * high memory region beyond 4GB).
 * This represents a compromise between how much RAM can be given to
 * a 32 bit VM and leaving space for expansion and in particular for PCI.
 * Note that devices should generally be placed at multiples of 0x10000,
 * to accommodate guests using 64K pages.
 */
static const MemMapEntry base_memmap[] = {

    // >>>>> ROBA <<<<<

    [VIRT_MMIO] =               { 0x0a000000, 0x00000200 },
    /* ...repeating for a total of NUM_VIRTIO_TRANSPORTS, each of that size */

    // Custom memory mapping for FFT Cusotm Accelerator Device
    [VIRT_FFT_ACC] =            { 0x0b000000, 0x00000100 },

    [VIRT_PLATFORM_BUS] =       { 0x0c000000, 0x02000000 },

    // >>>>> ROBA <<<<<

};

// >>>>> ROBA <<<<<

static const int a15irqmap[] = {

    // >>>>> ROBA <<<<<

    [VIRT_PLATFORM_BUS] = 112, /* ...to 112 + PLATFORM_BUS_NUM_IRQS -1 */

    [VIRT_FFT_ACC]  = 112 + PLATFORM_BUS_NUM_IRQS,
};

// >>>>> ROBA <<<<<

