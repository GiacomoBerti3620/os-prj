#!/bin/bash

cp fft_acc_drv.c ../poky/meta-osprj/recipes-osprj/osprj-mod/files
cp Makefile ../poky/meta-osprj/recipes-osprj/osprj-mod/files
cp osprj-mod_0.1.bb ../poky/meta-osprj/recipes-osprj/osprj-mod/
mkdir  ../poky/meta-osprj/recipes-osprj/osprj-testfiles
mkdir  ../poky/meta-osprj/recipes-osprj/osprj-testfiles/files
cp fft_test.c ../poky/meta-osprj/recipes-osprj/osprj-testfiles/files
cp base_test.c ../poky/meta-osprj/recipes-osprj/osprj-testfiles/files
cp osprj-testfiles_0.1.bb ../poky/meta-osprj/recipes-osprj/osprj-testfiles
