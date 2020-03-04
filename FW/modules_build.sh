#!/bin/sh
current_dir=${PWD}
modules_folder="${current_dir}/modules"
#library_type=$(grep -r "library_type=" build.sh | awk '{if($1=="library_type=1") {print 1} else {print 0}}')

#### Build modules ####
mkdir -p ${modules_folder}/build
cd ${modules_folder}/build

if [ ${ARCH} = "arm64" ]
then
    cmake -DCMAKE_BUILD_TYPE=Release -DARM64=ON -DCMAKE_TOOLCHAIN_FILE=../devel_rpi4_toolchain_64.cmake -DLIB_TYPE=ShareLib ..
else
    cmake -DCMAKE_BUILD_TYPE=Release -DARM64=OFF -DCMAKE_TOOLCHAIN_FILE=../devel_rpi4_toolchain_32.cmake -DLIB_TYPE=ShareLib ..
fi

make -j4
make install
cd ${current_dir}
