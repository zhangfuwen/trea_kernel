#ifndef ARCH_X86_PAGING_H
#define ARCH_X86_PAGING_H

#include <cstdint>

// 页表标志位
constexpr uint32_t PAGE_PRESENT = 0x1;        // 页面存在 (位0)
constexpr uint32_t PAGE_WRITE = 0x2;          // 可写 (位1)
constexpr uint32_t PAGE_USER = 0x4;           // 用户级 (位2)
constexpr uint32_t PAGE_WRITE_THROUGH = 0x8;  // 写透 (位3)
constexpr uint32_t PAGE_CACHE_DISABLE = 0x10; // 禁用缓存 (位4)
constexpr uint32_t PAGE_ACCESSED = 0x20;      // 已访问 (位5)
constexpr uint32_t PAGE_DIRTY = 0x40;         // 已修改 (位6)
constexpr uint32_t PAGE_GLOBAL = 0x100;       // 全局 (位8)
constexpr uint32_t PAGE_COW = 0x200;          // 写时复制 (位9), 系统自定义位

// 4M 以后开始分配内存
// 4M -> 4M + 4K 是PDT
// 4M + 4K -> 4M + 516K 是内存页表，为512K/4个条目，即128K*4K = 1024M = 1GB空间
// 以上是指物理内存，不需要虚拟地址
// 内存区域常量定义
namespace MemoryConstants
{
// 页大小
// constexpr uint32_t PAGE_SIZE = 4096;

// 物理内存区域大小（以页为单位）
constexpr uint32_t DMA_ZONE_START = 0x0;  // 0MB
constexpr uint32_t DMA_ZONE_END = 0x1000; // 16MB (4096页)
constexpr uint32_t NORMAL_ZONE_START = DMA_ZONE_END;
constexpr uint32_t NORMAL_ZONE_END = 0x38000; // 896MB (229376页)
constexpr uint32_t HIGH_ZONE_START = NORMAL_ZONE_END;
constexpr uint32_t HIGH_ZONE_END = 0x100000; // 4GB (1048576页)

// 虚拟地址空间布局
constexpr uint32_t KERNEL_DIRECT_MAP_START = 0xC0000000; // 3GB (内核空间起始地址)
constexpr uint32_t KERNEL_DIRECT_MAP_END =
    0xF8000000; // 3GB + 896MB (直接映射区，用于映射DMA_ZONE和NORMAL_ZONE)
constexpr uint32_t VMALLOC_START = KERNEL_DIRECT_MAP_END; // 3GB + 896MB
constexpr uint32_t VMALLOC_END = 0xF8000000; // 3GB + 896MB (VMALLOC区域，64MB，用于非连续内存分配)
constexpr uint32_t KMAP_START = 0xF8000000;  // 3GB + 896MB
constexpr uint32_t KMAP_END = 0xFC000000;    // 3GB + 960MB (KMAP区域，64MB，用于临时内核映射)
} // namespace MemoryConstants

// 4M 以后开始分配内存
// 4M -> 4M + 4K 是PDT
// 4M + 4K -> 4M + 516K 是内存页表，为512K/4个条目，即128K*4K = 1024M = 1GB空间
// 以上是指物理内存，不需要虚拟地

constexpr uint32_t PAGE_DIRECTORY_ADDR = 0x400000; // 4MB地址处是页目录
constexpr uint32_t K_FIRST_4M_PT = 0x401000;       // 4MB + 4KB地址处是前4M页表
constexpr uint32_t K_PAGE_TABLE_START = 0x402000;  // 4MB + 8KB地址处是页表
constexpr uint32_t K_PAGE_TABLE_COUNT = 224;       // 224 * 1024page/table * 4KB/page = 896MB

static const uint32_t PAGE_SIZE = 0x1000;      // 页面大小
static const uint32_t USER_START = 0x40000000; // 用户空间起始地址
static const uint32_t USER_END = 0xC0000000;   // 用户空间结束地址

#define TEMP_MAPPING_VADDR 0xFFC00000 // 内核临时映射专用地址

using PFN = uint32_t;
using VADDR = void*;
using PADDR = uint32_t;


// 页面状态标志
enum class PageFlags : uint32_t {
    PAGE_RESERVED = 1 << 0,   // 保留页面
    PAGE_ALLOCATED = 1 << 1,  // 已分配
    PAGE_DIRTY = 1 << 2,      // 脏页面
    PAGE_LOCKED = 1 << 3,     // 锁定页面
    PAGE_REFERENCED = 1 << 4, // 被引用
    PAGE_ACTIVE = 1 << 5,     // 活跃页面
    PAGE_INACTIVE = 1 << 6,   // 不活跃页面
};

// 物理页面描述符
struct page {
    uint32_t flags;           // 页面状态标志
    uint32_t _count;          // 引用计数
    uint32_t virtual_address; // 映射的虚拟地址
    uint32_t pfn;             // 页框号
    struct page* next;        // 链表指针，用于空闲页面链表
};

struct PageDirectory {
    uint32_t entries[1024];
} __attribute__((aligned(4096)));

struct PageTable {
    uint32_t entries[1024];
} __attribute__((aligned(4096)));

void printPDPTE(VADDR vaddr);
void printPDE(PageDirectory* pdVirt, uint32_t index);
void printPD(PageDirectory* pdVirt, uint32_t startIndex, uint32_t count);
void __printPDPTE(VADDR vaddr, PageDirectory* pdVirt);
void printPTEFlags(uint32_t pte);


void PagingValidate(PageDirectory * pd);
class PageManager
{
public:
    PageManager();
    void init();
    static void mapKernelSpace();
    /**
     * @brief 复制内存空间，使用写时复制技术
     * @param src 源页目录
     * @param dstPgd 目标页目录, out pointer
     * @return 0 成功，-1 失败
     */
    static int copyMemorySpaceCOW(PageDirectory* src, PageDirectory* dstPgd);

    static void loadPageDirectory(uint32_t dir);
    static void enablePaging();
    static void disablePaging();

    // 映射虚拟地址到物理地址
    void mapPage(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);
    void unmapPage(uint32_t virt_addr);

    // 获取页表项标志位
    uint32_t getPageFlags(uint32_t virt_addr);
    // 设置页表项标志位
    void setPageFlags(uint32_t virt_addr, uint32_t flags);

    // // 获取物理地址
    // uint32_t getPhysicalAddress(uint32_t virt_addr);

    // 获取当前页目录
    PageDirectory* getCurrentPageDirectory()
    {
        return curPgdVirt;
    }
    // 切换页目录
    void switchPageDirectory(PageDirectory* dirVirt, void* dirPhys);

private:
    static void copyKernelSpace(PageDirectory* src, PageDirectory* dst);
    PageDirectory* curPgdVirt;
};

#endif // ARCH_X86_PAGING_H