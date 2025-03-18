#!/bin/bash

cp ./qemu/hw/misc/virt_fft_acc.c ../qemu/hw/misc/
cp ./qemu/hw/misc/Kconfig ../qemu/hw/misc/
cp ./qemu/hw/misc/meson.build ../qemu/hw/misc/
cp ./qemu/hw/arm/virt.c ../qemu/arm/misc/
cp ./qemu/hw/arm/virt.h ../qemu/arm/misc/