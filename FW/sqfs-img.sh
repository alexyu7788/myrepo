#!/bin/sh

current_dir=${PWD}
temp_folder="${current_dir}/temp"
tmp_rootfs_folder="${current_dir}/tmp_rootfs"
fake_root_session_file="${current_dir}/backup.environ"
output_root_fs="${current_dir}/rootfs.squashfs"

if [ -f ${output_root_fs} ];then
    rm ${output_root_fs}
fi

if [ -d ${tmp_rootfs_folder} ];then
    rm -rf ${tmp_rootfs_folder}
fi

arm-linux-strip ${temp_folder}/usr/sbin/*
arm-linux-strip ${temp_folder}/usr/lib/*.so*

fakeroot -s ${fake_root_session_file} ${current_dir}/../SDK/Rootfs/squashfs-4.4/unsquashfs -d ${tmp_rootfs_folder} ${current_dir}/../SDK/Rootfs/binary/rootfs.squashfs

cp -aru ${temp_folder}/etc/* ${tmp_rootfs_folder}/etc
cp -ar ${temp_folder}/etc/init.d/rcS ${tmp_rootfs_folder}/etc/init.d/
cp -ar ${temp_folder}/usr/* ${tmp_rootfs_folder}/usr
cp -ar ${temp_folder}/lib/* ${tmp_rootfs_folder}/

fakeroot -i ${fake_root_session_file} -s ${fake_root_session_file} ${current_dir}/../SDK/Rootfs/squashfs-4.4/mksquashfs ${tmp_rootfs_folder} ${output_root_fs} -comp xz -noappend -all-root -b 524288

rm -rf ${temp_folder}
rm -rf ${tmp_rootfs_folder}
rm ${fake_root_session_file}
