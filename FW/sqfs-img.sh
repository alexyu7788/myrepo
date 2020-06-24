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

mkdir -p ${tmp_rootfs_folder}/home/alex
fakeroot -i ${fake_root_session_file} -s ${fake_root_session_file} chown -R 7788:7788 ${tmp_rootfs_folder}/home/alex
echo "alex:\$1\$Rjpavdix\$Yara09Rrq081IhCkbpV0L0:1:0:99999:7:::" >> ${tmp_rootfs_folder}/etc/shadow
echo "alex:x:7788:7788:Linux User,,,:/home/alex:/bin/sh" >> ${tmp_rootfs_folder}/etc/passwd
fakeroot -i ${fake_root_session_file} -s ${fake_root_session_file} chmod 660 ${tmp_rootfs_folder}/etc/sudoers
echo "alex ALL=(ALL) ALL" >> ${tmp_rootfs_folder}/etc/sudoers
fakeroot -i ${fake_root_session_file} -s ${fake_root_session_file} chmod 220 ${tmp_rootfs_folder}/etc/sudoers

fakeroot -i ${fake_root_session_file} -s ${fake_root_session_file} ${current_dir}/../SDK/Rootfs/squashfs-4.4/mksquashfs ${tmp_rootfs_folder} ${output_root_fs} -comp xz -noappend -all-root -b 524288

rm -rf ${temp_folder}
rm -rf ${tmp_rootfs_folder}
rm ${fake_root_session_file}
