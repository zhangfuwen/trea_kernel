#include "kernel/zone.h"

#include <lib/serial.h>

#include "kernel/kernel_memory.h"
#include "lib/debug.h"

Zone::Zone()
{
    // 初始化水位标记
}

void Zone::init(ZoneType type, uint32_t start_pfn, uint32_t end_pfn)
{
    this->type = type;
    zone_start_pfn = start_pfn;
    zone_end_pfn = end_pfn;
    size = end_pfn - start_pfn;
    nr_free_pages = end_pfn - start_pfn;
    // 初始化区域的其他资源
    // 这里可以添加额外的初始化逻辑
    watermark[static_cast<int>(WatermarkLevel::WMARK_MIN)] = size / 16; // 6.25%
    watermark[static_cast<int>(WatermarkLevel::WMARK_LOW)] = size / 8;  // 12.5%
    watermark[static_cast<int>(WatermarkLevel::WMARK_HIGH)] = size / 4; // 25%

    // 初始化伙伴系统分配器
    buddy_allocator.init(zone_start_pfn * 4096, size * 4096);
}

uint32_t Zone::allocPages(uint32_t gfp_mask, uint32_t order)
{
    uint32_t count = 1 << order;
    if(count > nr_free_pages) {
        return 0; // 返回0表示分配失败
    }

    uint32_t allocated_addr = buddy_allocator.allocate_pages(gfp_mask, order);
    if(allocated_addr == 0) {
        return 0;
    }

    nr_free_pages -= count;
    return allocated_addr / 4096; // 转换为页帧号
}

void Zone::freePages(uint32_t pfn, uint32_t order)
{
    uint32_t count = 1 << order;
    // 检查pfn是否在当前区域范围内
    if(pfn < zone_start_pfn || pfn + count > zone_end_pfn) {
        return;
    }

    buddy_allocator.free_pages(pfn * 4096, order);
    nr_free_pages += count;
}

void Zone::decRefPage(uint32_t pfn)
{
    if(pfn < zone_start_pfn || pfn >= zone_end_pfn) {
        return;
    }
    buddy_allocator.decrement_ref_count(pfn * 4096);
}

void Zone::increment_ref_count(uint32_t pfn)
{
    if(pfn < zone_start_pfn || pfn >= zone_end_pfn) {
        return;
    }
    buddy_allocator.increment_ref_count(pfn * 4096);
}



uint32_t Zone::getFreePages() const
{
    return nr_free_pages;
}

void Zone::setWatermark(WatermarkLevel level, uint32_t value)
{
    if(value > size) {
        value = size;
    }
    watermark[static_cast<int>(level)] = value;
}

uint32_t Zone::getWatermark(WatermarkLevel level) const
{
    return watermark[static_cast<int>(level)];
}

bool Zone::isWatermarkReached(WatermarkLevel level) const
{
    return nr_free_pages <= watermark[static_cast<int>(level)];
}

bool Zone::migratePagesTo(Zone* target, uint32_t count)
{
    // NORMAL区域不支持页面迁移
    if(type == ZoneType::ZONE_NORMAL || target->type == ZoneType::ZONE_NORMAL) {
        return false;
    }

    if(!target || count > nr_free_pages || count > target->size - target->nr_free_pages) {
        return false;
    }

    // 计算所需的order
    uint32_t order = 0;
    while ((1u << order) < count) {
        order++;
    }

    // 执行页面迁移
    uint32_t source_pfn = allocPages(0, order);
    if(source_pfn == 0) {
        return false;
    }

    // 在目标区域分配页面
    uint32_t target_pfn = target->allocPages(0, order);
    if(target_pfn == 0) {
        // 如果目标分配失败，恢复源区域
        freePages(source_pfn, order);
        return false;
    }

    // 这里需要实现实际的页面数据迁移
    // 暂时只更新计数器

    return true;
}