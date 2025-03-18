#!/bin/bash

cp echodev-cmd.h ../poky/meta-osprj/recipes-osprj/osprj-mod/files
cp echodev-drv.c ../poky/meta-osprj/recipes-osprj/osprj-mod/files
cp Makefile ../poky/meta-osprj/recipes-osprj/osprj-mod/files
cp osprj-mod_0.1.bb ../poky/meta-osprj/recipes-osprj/osprj-mod/
mkdir  ../poky/meta-osprj/recipes-osprj/osprj-testfiles
mkdir  ../poky/meta-osprj/recipes-osprj/osprj-testfiles/files
cp bar0_test.c ../poky/meta-osprj/recipes-osprj/osprj-testfiles/files
cp bar1_test.c ../poky/meta-osprj/recipes-osprj/osprj-testfiles/files
cp osprj-testfiles_0.1.bb ../poky/meta-osprj/recipes-osprj/osprj-testfiles
