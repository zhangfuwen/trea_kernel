#include "drivers/block_device.h"
#include <lib/string.h>
#include "lib/ioport.h"

namespace kernel {

RamDisk::RamDisk(size_t size_in_bytes) {
    // 初始化设备信息
    info.block_size = 512;  // 标准块大小
    info.total_blocks = size_in_bytes / info.block_size;
    info.read_only = false;

    // 分配内存
    disk_data = new uint8_t[size_in_bytes];
    memset(disk_data, 0, size_in_bytes);
}

RamDisk::~RamDisk() {
    if (disk_data) {
        delete[] disk_data;
        disk_data = nullptr;
    }
}

const BlockDeviceInfo& RamDisk::get_info() const {
    return info;
}

bool RamDisk::read_block(uint32_t block_num, void* buffer) {
    if (block_num >= info.total_blocks || !buffer) {
        return false;
    }

    size_t offset = block_num * info.block_size;
    memcpy(buffer, disk_data + offset, info.block_size);
    return true;
}

bool RamDisk::write_block(uint32_t block_num, const void* buffer) {
    if (block_num >= info.total_blocks || !buffer || info.read_only) {
        return false;
    }

    size_t offset = block_num * info.block_size;
    memcpy(disk_data + offset, buffer, info.block_size);
    return true;
}

void RamDisk::sync() {
    // 内存设备不需要同步操作
}

DiskDevice::DiskDevice() {
    // 初始化设备信息
    info.block_size = 512;  // 标准扇区大小
    info.total_blocks = 0;  // 总块数在init时设置
    info.read_only = false;
}

DiskDevice::~DiskDevice() {
}

bool DiskDevice::init() {
    // 获取磁盘大小（以字节为单位）
    // 假设磁盘大小为100MB
    size_t disk_size = 100 * 1024 * 1024;
    info.total_blocks = disk_size / info.block_size;
    return true;
}

const BlockDeviceInfo& DiskDevice::get_info() const {
    return info;
}

bool DiskDevice::read_block(uint32_t block_num, void* buffer) {
    if (block_num >= info.total_blocks || !buffer) {
        return false;
    }

    // 计算扇区偏移
    size_t offset = block_num * info.block_size;
    
    // 设置LBA地址
    outb(0x1F6, 0xE0 | ((block_num >> 24) & 0x0F));
    outb(0x1F2, 1);                           // 扇区数
    outb(0x1F3, block_num & 0xFF);
    outb(0x1F4, (block_num >> 8) & 0xFF);
    outb(0x1F5, (block_num >> 16) & 0xFF);
    
    // 发送读命令
    outb(0x1F7, 0x20);
    
    // 等待数据准备好
    while ((inb(0x1F7) & 0x88) != 0x08);
    
    // 读取数据
    for (size_t i = 0; i < info.block_size; i += 2) {
        uint16_t data = inw(0x1F0);
        ((uint8_t*)buffer)[i] = data & 0xFF;
        ((uint8_t*)buffer)[i + 1] = (data >> 8) & 0xFF;
    }
    
    return true;
}

bool DiskDevice::write_block(uint32_t block_num, const void* buffer) {
    if (block_num >= info.total_blocks || !buffer || info.read_only) {
        return false;
    }

    // 计算扇区偏移
    size_t offset = block_num * info.block_size;
    
    // 设置LBA地址
    outb(0x1F6, 0xE0 | ((block_num >> 24) & 0x0F));
    outb(0x1F2, 1);                           // 扇区数
    outb(0x1F3, block_num & 0xFF);
    outb(0x1F4, (block_num >> 8) & 0xFF);
    outb(0x1F5, (block_num >> 16) & 0xFF);
    
    // 发送写命令
    outb(0x1F7, 0x30);
    
    // 等待磁盘准备好
    while ((inb(0x1F7) & 0x88) != 0x08);
    
    // 写入数据
    for (size_t i = 0; i < info.block_size; i += 2) {
        uint16_t data = ((uint8_t*)buffer)[i] | (((uint8_t*)buffer)[i + 1] << 8);
        outw(0x1F0, data);
    }
    
    // 等待写入完成
    while (inb(0x1F7) & 0x80);
    
    return true;
}

void DiskDevice::sync() {
    // TODO: 实现磁盘缓存同步操作
}

} // namespace kernel