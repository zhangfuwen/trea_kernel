#pragma once
#include <stddef.h>
#include <stdint.h>

namespace kernel {

// 块设备的基本参数
struct BlockDeviceInfo {
    size_t block_size;      // 块大小（字节）
    size_t total_blocks;    // 总块数
    bool   read_only;       // 是否只读
};

// 块设备驱动接口
class BlockDevice {
public:
    BlockDevice() = default;
    virtual ~BlockDevice() = default;

    // 获取设备信息
    virtual const BlockDeviceInfo& get_info() const = 0;

    // 读取指定块
    virtual bool read_block(uint32_t block_num, void* buffer) = 0;

    // 写入指定块
    virtual bool write_block(uint32_t block_num, const void* buffer) = 0;

    // 同步缓存数据到设备
    virtual void sync() = 0;
};

// 内存模拟的块设备实现
class RamDisk : public BlockDevice {
public:
    RamDisk(size_t size_in_bytes);
    virtual ~RamDisk();

    virtual const BlockDeviceInfo& get_info() const override;
    virtual bool read_block(uint32_t block_num, void* buffer) override;
    virtual bool write_block(uint32_t block_num, const void* buffer) override;
    virtual void sync() override;

private:
    uint8_t* disk_data;
    BlockDeviceInfo info;
};

// 物理磁盘设备实现
class DiskDevice : public BlockDevice {
public:
    DiskDevice();
    virtual ~DiskDevice();

    // 初始化磁盘设备
    bool init();

    virtual const BlockDeviceInfo& get_info() const override;
    virtual bool read_block(uint32_t block_num, void* buffer) override;
    virtual bool write_block(uint32_t block_num, const void* buffer) override;
    virtual void sync() override;

private:
    BlockDeviceInfo info;
};

} // namespace kernel
