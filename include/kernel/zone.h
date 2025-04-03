#ifndef KERNEL_ZONE_H
#define KERNEL_ZONE_H

#include "kernel/buddy_allocator.h"
#include <cstdint>

// 内存区域类型
enum class ZoneType {
    ZONE_DMA,    // DMA可用的内存区域
    ZONE_NORMAL, // 普通内存区域
    ZONE_HIGH    // 高端内存区域
};

// 内存水位级别
enum class WatermarkLevel {
    WMARK_MIN, // 最小水位
    WMARK_LOW, // 低水位
    WMARK_HIGH // 高水位
};

class Zone
{
public:
    // 构造函数
    Zone();

    // 初始化区域
    void init(ZoneType type, uint32_t start_pfn, uint32_t end_pfn);

    // 分配页面
    uint32_t allocPages(uint32_t gfp_mask, uint32_t order);

    // 释放页面
    void freePages(uint32_t pfn, uint32_t order);
    void decRefPage(uint32_t pfn);
    void increment_ref_count(uint32_t pfn);

    // 获取区域空闲页面数量
    uint32_t getFreePages() const;

    // 设置水位标记
    void setWatermark(WatermarkLevel level, uint32_t value);

    // 获取水位标记
    uint32_t getWatermark(WatermarkLevel level) const;

    // 检查是否达到水位标记
    bool isWatermarkReached(WatermarkLevel level) const;

    // 迁移页面到其他区域
    bool migratePagesTo(Zone* target, uint32_t count);

    // 获取区域类型
    ZoneType getType() const
    {
        return type;
    }

private:
    ZoneType type;                  // 区域类型
    uint32_t nr_free_pages;         // 空闲页面数量
    uint32_t zone_num;              // 空闲页面数量
    uint32_t zone_start_pfn;        // 区域起始页帧号
    uint32_t zone_end_pfn;          // 区域结束页帧号
    uint32_t size;                  // 区域大小（以页为单位）
    uint32_t watermark[3];          // 水位标记
    BuddyAllocator buddy_allocator; // 伙伴系统分配器
};

#endif // KERNEL_ZONE_H