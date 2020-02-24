#!/bin/sh

current_dir=${PWD}
modules_folder="${current_dir}/modules"
#library_type=$(grep -r "library_type=" build.sh | awk '{if($1=="library_type=1") {print 1} else {print 0}}')

#### Build modules ####
mkdir -p ${modules_folder}/build
cd ${modules_folder}/build

#if [ ${library_type} -eq 0 ]; then
    cmake .. -DCMAKE_BUILD_TYPE=Release -DPLATFORM_TYPE=SCHUBERT -DLIB_TYPE=ShareLib  -DCMAKE_TOOLCHAIN_FILE=../devel_rpi4_toolchain.cmake
#else
#    cmake .. -DCMAKE_BUILD_TYPE=Release -DPLATFORM_TYPE=SCHUBERT -DLIB_TYPE=StaticLib -DCMAKE_TOOLCHAIN_FILE=../devel_rpi5_toolchain.cmake
#fi

make -j4
cd ${current_dir}
