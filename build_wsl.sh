#!/bin/bash

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo "选项:"
    echo "  -h, --help     显示帮助信息"
    echo "  -s, --skip-update  跳过apt update步骤"
    echo "  -j N           设置编译线程数为N (例如: -j1 使用单线程编译)"
}

# 解析命令行参数
SKIP_UPDATE=false
JOBS=$(nproc)
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -s|--skip-update)
            SKIP_UPDATE=true
            shift
            ;;
        -j*)
            if [[ "$1" == "-j" ]]; then
                JOBS="$2"
                shift 2
            else
                JOBS="${1#-j}"
                shift
            fi
            ;;
        *)
            echo "错误: 未知选项 $1"
            show_help
            exit 1
            ;;
    esac
done

# 安装必要的开发工具
echo "正在安装开发工具..."
if [ "$SKIP_UPDATE" = false ]; then
    echo "正在更新软件包列表..."
    sudo apt-get update
fi
sudo apt-get install -y build-essential nasm cmake xorriso grub-pc-bin grub-common gcc-multilib g++-multilib

# 创建并进入构建目录
echo "创建构建目录..."
mkdir -p build
cd build

# 运行CMake配置
echo "配置CMake..."
cmake .. \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_ASM_NASM_COMPILER=nasm \
    -DCMAKE_ASM_NASM_FLAGS="-f elf32"

# 编译项目
echo "编译项目..."
make -j$JOBS

# 创建ISO镜像
echo "创建ISO镜像..."
make kernel.iso

echo "构建完成！"
echo "ISO镜像位置: $(pwd)/kernel.iso"