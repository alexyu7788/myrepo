#!/bin/sh

path=${PWD}

echo del ${path}/apps/build
rm -rf ${path}/apps/build

echo del ${path}/apps/bin/
rm -rf ${path}/apps/bin/

echo del ${path}/apps/lib/
rm -rf ${path}/apps/lib

echo del ${path}/temp
rm -rf ${path}/temp

