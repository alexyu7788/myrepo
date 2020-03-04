#!/bin/sh

#
#make sure you make setup compiler environment before building codes.
#
echo "=->enter Kernel"
cd ./SDK/Kernel/

echo "=->make Kernel"
make bcm2711_defconfig
#make clean
make -j4
make dtbs
#cat arch/arm/boot/zImage arch/arm/boot/dts/r5s-spi-nand.dtb > zImage.dtb
cp arch/arm64/boot/Image arch/arm64/boot/dts/broadcom/bcm2711-rpi-4-b.dtb ../../Images_64
echo "=->leave kernel"
cd ../..

#Now check the generated Kernel Image
echo "=->build done"

