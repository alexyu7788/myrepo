#!/bin/sh

current_dir=${PWD}
modules_folder="${current_dir}/modules"
apps_folder="${current_dir}/apps"
temp_folder="${current_dir}/temp"
patch_folder="${current_dir}/patch"


rm -rf ${temp_folder}
mkdir -p ${temp_folder}/etc
mkdir -p ${temp_folder}/usr/bin
mkdir -p ${temp_folder}/usr/lib
mkdir -p ${temp_folder}/usr/lib/plugins
mkdir -p ${temp_folder}/usr/sbin

#### Copy the libraries etc. from the modules ####
cp -a ${modules_folder}/install_temp_modules/bin/* ${temp_folder}/usr/bin
cp -a ${modules_folder}/install_temp_modules/lib/*.so* ${temp_folder}/usr/lib
cp -ar ${modules_folder}/install_temp_modules/lib/plugins/*.so*  ${temp_folder}/usr/lib/plugins
cp -ar ${modules_folder}/install_temp_modules/lib/pkgconfig ${temp_folder}/usr/lib/pkgconfig/

#### Copy the bin etc. from the apps ####
cp -a ${apps_folder}/bin/* ${temp_folder}/usr/sbin

#### Copy the patch file. from the patch folder ####
cp -ar ${patch_folder}/* ${temp_folder}/

#### copy default setting ####
#if [ ! -d  ${temp_folder}/etc/default_system ];then
#    mkdir -p ${temp_folder}/etc/default_system
#fi
#cp ${temp_folder}/etc/system_setting.ini ${temp_folder}/etc/default_system

