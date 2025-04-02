#!/bin/bash

# 接受一个参数，即要编译的目录
if [[ $# -eq 0 ]]; then
    echo "请提供要编译的目录作为参数"
    exit 1
fi
# 编译目录
BUILD_DIR="$1"
# 进入编译目录
cd "$BUILD_DIR"

cd rootfs_bin
ls -la ../rootfs.cpio
if [[ -f ../rootfs.cpio ]]; then
    rm ../rootfs.cpio
fi
find . -print | cpio -ov --format=newc > ../rootfs.cpio
ls -la ../rootfs.cpio
cd -