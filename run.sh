#!/bin/bash
# 输入参数
if [ $# -lt 2 ]; then
    echo "Usage: $0 <iso_file> <disk_image>"
    echo "using test.img"
fi
# 定义变量
iso_file=$1

if [ $# -eq 1 ]; then
    disk_image="test.img"
else
    disk_image=$2
fi

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
# 运行 QEMU 并启用详细调试日志，输出到文件以便后续分析
# 启用多个日志类别以记录尽可能多的信息
# 注意：QEMU 默认日志不包含时间戳，可以通过外部工具（如 ts 或 logger）添加时间戳
# 来源信息（如 CPU ID）可能在某些日志类别中包含
qemu-system-i386 \
    -cdrom $iso_file \
    -hda $disk_image \
    -m 1G \
    -serial stdio  \
    -smp cores=4 \
    -s 2>&1 | tee qemu.log

