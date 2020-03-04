#!/bin/sh
ARM64=ON
echo $(export | grep "ARCH=")

#ARM64=$(export | grep "ARCH=" | awk '{ if ($1 == "declare -x ARCH="arm64"") print "ON"; else print "OFF"}')
#echo $ARM64

#current_dir=${PWD}
#modules_folder="${current_dir}/modules"
##library_type=$(grep -r "library_type=" build.sh | awk '{if($1=="library_type=1") {print 1} else {print 0}}')
#
##### Build modules ####
#mkdir -p ${modules_folder}/build
#cd ${modules_folder}/build
#
#cmake -DCMAKE_BUILD_TYPE=Release -DARM64=$ARM64 -DCMAKE_TOOLCHAIN_FILE=../devel_rpi4_toolchain_64.cmake -DLIB_TYPE=ShareLib ..
#
#make -j4
#make install
#cd ${current_dir}
