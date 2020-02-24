#!/bin/sh

#
#make sure you make setup compiler environment before building codes.
#
echo "enter Bootloader"
cd SDK/Bootloader/u-boot-2020.01

echo "compile U-boot"
make distclean
make rpi_4_defconfig
make clean
make -j4

echo "Copy the generated files"
cp u-boot.bin ../../../Images

echo "=->leave Bootloader"
cd ../../../
echo "=->build done"

