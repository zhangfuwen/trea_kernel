#!/bin/bash

# 检查参数
if [ $# -lt 1 ]; then
    echo "Usage: $0 <disk_image_file>"
    exit 1
fi

# 定义变量
disk_image=$1
disk_size=100M

# 创建空的磁盘镜像文件
dd if=/dev/zero of=$disk_image bs=1M count=100

# 格式化为ext2文件系统
mkfs.ext2 $disk_image

# 创建临时挂载点
tmp_mount=$(mktemp -d)

# 挂载磁盘镜像
sudo mount -o loop $disk_image $tmp_mount

# 检查rootfs/binary目录是否存在
if [ -d "rootfs/binary" ]; then
    # 复制文件到磁盘镜像
    sudo cp -r rootfs/binary/* $tmp_mount/
else
    echo "错误：rootfs/binary目录不存在"
fi

# 卸载磁盘镜像
sudo umount $tmp_mount

# 删除临时挂载点
rmdir $tmp_mount

echo "磁盘镜像创建完成：$disk_image"