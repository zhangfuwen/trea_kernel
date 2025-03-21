#!/bin/bash

# 输入参数
if [ $# -lt 1 ]; then
    echo "Usage: $0 <iso_file>"
    exit 1
fi
# 定义变量
iso_file=$1

# 检查qemu-system-i386是否已安装
if ! command -v qemu-system-i386 &> /dev/null; then
    echo "qemu-system-i386未安装，正在安装..."
    if command -v apt &> /dev/null; then
        sudo apt update
        sudo apt install -y qemu-system-x86
    else
        echo "错误：无法找到apt包管理器，请手动安装qemu-system-i386"
        exit 1
    fi
fi

# 检查ISO文件是否存在
if [ ! -f $iso_file ]; then
    echo "错误：找不到文件:$iso_file，请先构建项目"
    exit 1
fi

# 运行内核
echo "启动CustomKernel..."
qemu-system-i386 \
    -cdrom $iso_file \
    -m 512M \
    -serial stdio  \
    -s