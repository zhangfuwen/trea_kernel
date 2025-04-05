#pragma once
#include <cstdint>

// 红黑树节点颜色
enum class Color { RED, BLACK };

// 虚拟内存区域节点
struct VmArea {
    uint32_t start_addr; // 起始地址
    uint32_t size;       // 区域大小
    Color color;         // 节点颜色
    VmArea* parent;      // 父节点
    VmArea* left;        // 左子节点
    VmArea* right;       // 右子节点

    VmArea(uint32_t start, uint32_t sz)
        : start_addr(start), size(sz), color(Color::RED), parent(nullptr), left(nullptr),
          right(nullptr)
    {
    }
};

// 虚拟内存红黑树
class VirtualMemoryTree
{
public:
    VirtualMemoryTree(uint32_t start, uint32_t end);
    ~VirtualMemoryTree();

    // 分配指定大小的虚拟内存区域
    uint32_t allocate(uint32_t size);

    // 释放指定地址的虚拟内存区域
    void free(uint32_t addr);

    // 获取可用内存大小
    uint32_t get_free_size() const;

private:
    VmArea* root;            // 根节点
    uint32_t start_addr;     // 起始地址
    uint32_t end_addr;       // 结束地址
    uint32_t total_size;     // 总大小
    uint32_t allocated_size; // 已分配大小

    // 红黑树操作
    void left_rotate(VmArea* node);
    void right_rotate(VmArea* node);
    void insert_fixup(VmArea* node);
    void delete_fixup(VmArea* node);
    void transplant(VmArea* u, VmArea* v);
    VmArea* minimum(VmArea* node);

    // 查找合适的空闲区域
    VmArea* find_free_area(uint32_t size);

    // 清理红黑树
    void cleanup(VmArea* node);
};