


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
    [VIRT_FFT_ACC] =            { 0x0b000000, 0x00000200 },

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

static void create_virtio_devices(const VirtMachineState *vms)
{
    int i;
    hwaddr size = vms->memmap[VIRT_MMIO].size;
    MachineState *ms = MACHINE(vms);


    // >>>>> ROBA <<<<<

}

static void create_virt_fft_acc_device(const VirtMachineState *vms)
{
    hwaddr base = vms->memmap[VIRT_FFT_ACC].base;
    hwaddr size = vms->memmap[VIRT_FFT_ACC].size;
    int irq = vms->irqmap[VIRT_FFT_ACC];
    char *nodename;

    sysbus_create_simple("virt-fft-acc", base, qdev_get_gpio_in(vms->gic, irq));

    nodename = g_strdup_printf("/virt_fft_acc@%" PRIx64, base);
    qemu_fdt_add_subnode(vms->fdt, nodename);
    qemu_fdt_setprop_string(vms->fdt, nodename, "compatible", "virt-fft-acc");
    qemu_fdt_setprop_sized_cells(vms->fdt, nodename, "reg", 2, base, 2, size);
    qemu_fdt_setprop_cells(vms->fdt, nodename, "interrupt-parent",
                           vms->gic_phandle);
    qemu_fdt_setprop_cells(vms->fdt, nodename, "interrupts",
                           GIC_FDT_IRQ_TYPE_SPI, irq,
                           GIC_FDT_IRQ_FLAGS_LEVEL_HI);

    g_free(nodename);
}

// >>>>> ROBA <<<<<

static void machvirt_init(MachineState *machine)
{
    
    // >>>>> ROBA <<<<<

    /* Create mmio transports, so the user can create virtio backends
     * (which will be automatically plugged in to the transports). If
     * no backend is created the transport will just sit harmlessly idle.
     */
    create_virtio_devices(vms);


    // ADD CUSTOM FFT ACC DEVICE
    create_virt_fft_acc_device(vms);


    vms->fw_cfg = create_fw_cfg(vms, &address_space_memory);
    rom_set_fw(vms->fw_cfg);

    create_platform_bus(vms);

    // >>>>> ROBA <<<<<

}

// >>>>> ROBA <<<<<