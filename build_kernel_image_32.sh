#!/bin/sh

#
#make sure you make setup compiler environment before building codes.
#
echo "=->enter Kernel"
cd ./SDK/Kernel/

echo "=->make Kernel"
make bcm2711_local_defconfig
#make bcm2711_defconfig
#make clean
make -j4
make dtbs
cp arch/arm/boot/zImage arch/arm/boot/dts/bcm2711-rpi-4-b.dtb ../../Images_32
echo "=->leave kernel"
cd ../..

#Now check the generated Kernel Image
echo "=->build done"

