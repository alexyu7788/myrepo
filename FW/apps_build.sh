#!/bin/sh

current_dir=${PWD}
apps_folder="${current_dir}/apps"
#library_type=$(grep -r "library_type=" build.sh | awk '{if($1=="library_type=1") {print 1} else {print 0}}')

#### Build modules ####
mkdir -p ${apps_folder}/build
cd ${apps_folder}/build

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../devel_rpi4_toolchain.cmake -DLIB_TYPE=ShareLib ..

make -j4
cd ${current_dir}
