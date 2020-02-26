#!/bin/sh

path=${PWD}

echo del ${path}/modules/bin
rm -rf ${path}/modules/bin

echo del ${path}/modules/build
rm -rf ${path}/modules/build

echo del ${path}/modules/install_temp_modules/
rm -rf ${path}/modules/install_temp_modules/

echo del ${path}/modules/libs/
rm -rf ${path}/modules/libs/
