#!/usr/bin/env python3
import os
import struct
import sys

# CPIO格式的魔数
CPIO_MAGIC = 0x070701

# CPIO头部结构
class CPIOHeader:
    def __init__(self, name, mode, size):
        self.magic = CPIO_MAGIC
        self.ino = 0
        self.mode = mode
        self.uid = 0
        self.gid = 0
        self.nlink = 1
        self.mtime = 0
        self.filesize = size
        self.devmajor = 0
        self.devminor = 0
        self.rdevmajor = 0
        self.rdevminor = 0
        self.namesize = len(name) + 1
        self.check = 0

    def pack(self):
        return struct.pack('<6I8I', 
            self.magic, self.ino, self.mode, self.uid, self.gid, self.nlink,
            self.mtime, self.filesize, self.devmajor, self.devminor,
            self.rdevmajor, self.rdevminor, self.namesize, self.check)

def align(n, size=4):
    return (n + size - 1) & ~(size - 1)

def make_cpio(input_dir, output_file):
    with open(output_file, 'wb') as out:
        # 遍历目录
        for root, dirs, files in os.walk(input_dir):
            # 处理目录
            for name in dirs:
                path = os.path.join(root, name)
                relpath = os.path.relpath(path, input_dir)
                header = CPIOHeader(relpath, 0o040755, 0)
                out.write(header.pack())
                out.write(relpath.encode() + b'\0')
                # 对齐到4字节边界
                pos = out.tell()
                if pos % 4:
                    out.write(b'\0' * (4 - pos % 4))

            # 处理文件
            for name in files:
                path = os.path.join(root, name)
                relpath = os.path.relpath(path, input_dir)
                size = os.path.getsize(path)
                header = CPIOHeader(relpath, 0o100644, size)
                out.write(header.pack())
                out.write(relpath.encode() + b'\0')
                # 对齐文件名到4字节边界
                pos = out.tell()
                if pos % 4:
                    out.write(b'\0' * (4 - pos % 4))
                # 写入文件内容
                with open(path, 'rb') as f:
                    out.write(f.read())
                # 对齐文件内容到4字节边界
                pos = out.tell()
                if pos % 4:
                    out.write(b'\0' * (4 - pos % 4))

        # 写入TRAILER
        trailer = CPIOHeader('TRAILER!!!', 0, 0)
        out.write(trailer.pack())
        out.write(b'TRAILER!!!\0')
        # 对齐最终文件到4字节边界
        pos = out.tell()
        if pos % 4:
            out.write(b'\0' * (4 - pos % 4))

def main():
    if len(sys.argv) != 3:
        print(f'Usage: {sys.argv[0]} <input_dir> <output_file>')
        sys.exit(1)

    input_dir = sys.argv[1]
    output_file = sys.argv[2]

    if not os.path.isdir(input_dir):
        print(f'Error: {input_dir} is not a directory')
        sys.exit(1)

    make_cpio(input_dir, output_file)
    print(f'Created CPIO archive: {output_file}')

if __name__ == '__main__':
    main()