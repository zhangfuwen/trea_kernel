#include "drivers/block_device.h"
#include "lib/ioport.h"
#include <lib/string.h>

#include <lib/debug.h>

namespace kernel {

RamDisk::RamDisk(size_t size_in_bytes) {
    // 初始化设备信息
    info.sector_size = 512;  // 标准块大小
    info.total_sectors = size_in_bytes / info.sector_size;
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
    if (block_num >= info.total_sectors || !buffer) {
        return false;
    }

    size_t offset = block_num * info.sector_size;
    memcpy(buffer, disk_data + offset, info.sector_size);
    return true;
}

bool RamDisk::write_block(uint32_t block_num, const void* buffer) {
    if (block_num >= info.total_sectors || !buffer || info.read_only) {
        return false;
    }

    size_t offset = block_num * info.sector_size;
    memcpy(disk_data + offset, buffer, info.sector_size);
    return true;
}

void RamDisk::sync() {
    // 内存设备不需要同步操作
}

DiskDevice::DiskDevice() {
    // 初始化设备信息
    info.sector_size = 512;  // 标准扇区大小
    info.total_sectors = 0;  // 总块数在init时设置
    info.read_only = false;
}

DiskDevice::~DiskDevice() {
}

bool DiskDevice::init() {
    // 发送ATA IDENTIFY命令
    outb(0x1F6, 0xE0);  // LBA主设备
    outb(0x1F7, 0xEC);  // IDENTIFY命令

    // 等待命令就绪
    if ((inb(0x1F7) & 0x88) != 0x08) {
        log_err("IDENTIFY command failed\n");
        return false;
    }

    // 读取512字节的IDENTIFY数据
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(0x1F0);
    }

    // 解析最大LBA值（WORD 60-61为LBA28，WORD 100-103为LBA48）
    uint32_t lba28 = (identify_data[61] << 16) | identify_data[60];
    uint64_t lba48 = ((uint64_t)identify_data[103] << 48) |
                    ((uint64_t)identify_data[102] << 32) |
                    ((uint64_t)identify_data[101] << 16) |
                    identify_data[100];

    // 优先使用LBA48值
    uint64_t max_lba = lba48 ? lba48 + 1 : lba28 + 1;

    // 设置真实容量
    block_size = 4096;  // 使用标准扇区尺寸
    info.total_sectors = max_lba;
    info.sector_size = 512;
    
    log_info("Detected disk size: %llu sectors (%llu MB)\n", 
             max_lba, (max_lba * 512) / (1024 * 1024));
    return true;
}

const BlockDeviceInfo& DiskDevice::get_info() const {
    return info;
}
bool DiskDevice::read_block(uint32_t block_num, void* buffer)
{
    auto sectors_per_block = block_size / info.sector_size;
    auto sector_num = block_num * sectors_per_block;
    for(int i = 0; i < sectors_per_block; i++) {
        if(!read_sector(sector_num + i, (uint8_t*)buffer + i * info.sector_size)) {
            return false;
        }
    }
    return true;
}

bool DiskDevice::read_sector(uint32_t sector_num, void* buffer) {
    // debug_info("read_block\n");
    if (sector_num >= info.total_sectors || !buffer) {
        log_err("Invalid block number(%d, 0x%x) or buffer:0x%x\n", sector_num, buffer, buffer);
        return false;
    }

    // 计算扇区偏移
    size_t offset = sector_num * info.sector_size;

    // 设置LBA地址
    outb(0x1F6, 0xE0 | ((sector_num >> 24) & 0x0F));
    outb(0x1F2, 1);                           // 扇区数
    outb(0x1F3, sector_num & 0xFF);
    outb(0x1F4, (sector_num >> 8) & 0xFF);
    outb(0x1F5, (sector_num >> 16) & 0xFF);

    // 发送读命令
    outb(0x1F7, 0x20);

    // 等待数据准备好
    while ((inb(0x1F7) & 0x88) != 0x08);
    // debug_debug("disk realy ready!\n");

    // 读取数据
    // debug_debug("Reading sector 0x%x, size:%d(0x%x)\n", sector_num, info.sector_size, info.sector_size);
    for (size_t i = 0; i < info.sector_size; i += 2) {
        uint16_t data = inw(0x1F0);
        ((uint8_t*)buffer)[i] = data & 0xFF;
        ((uint8_t*)buffer)[i + 1] = (data >> 8) & 0xFF;
    }

    return true;
}
bool DiskDevice::write_block(uint32_t block_num, const void* buffer)
{
    auto sectors_per_block = block_size / info.sector_size;
    auto sector_num = block_num * sectors_per_block;
    for(int i = 0; i < sectors_per_block; i++) {
        if(!write_sector(sector_num + i, (uint8_t*)buffer + i * info.sector_size)) {
            return false;
        }
    }
    return true;
}

bool DiskDevice::write_sector(uint32_t sector_num, const void* buffer) {
    if (sector_num >= info.total_sectors || !buffer || info.read_only) {
        return false;
    }

    // 计算扇区偏移
    size_t offset = sector_num * info.sector_size;
    
    // 设置LBA地址
    outb(0x1F6, 0xE0 | ((sector_num >> 24) & 0x0F));
    outb(0x1F2, 1);                           // 扇区数
    outb(0x1F3, sector_num & 0xFF);
    outb(0x1F4, (sector_num >> 8) & 0xFF);
    outb(0x1F5, (sector_num >> 16) & 0xFF);
    
    // 发送写命令
    outb(0x1F7, 0x30);
    
    // 等待磁盘准备好
    while ((inb(0x1F7) & 0x88) != 0x08);
    
    // 写入数据
    for (size_t i = 0; i < info.sector_size; i += 2) {
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