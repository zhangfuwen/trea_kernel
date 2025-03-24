#include "kernel/virtual_memory_tree.h"

VirtualMemoryTree::VirtualMemoryTree(uint32_t start, uint32_t end)
    : root(nullptr), start_addr(start), end_addr(end),
      total_size(end - start), allocated_size(0) {
    // 创建一个表示整个空闲空间的节点
    root = new VmArea(start, total_size);
    root->color = Color::BLACK;
}

VirtualMemoryTree::~VirtualMemoryTree() {
    cleanup(root);
}

// 左旋操作
void VirtualMemoryTree::left_rotate(VmArea* x) {
    VmArea* y = x->right;
    x->right = y->left;
    
    if (y->left)
        y->left->parent = x;
        
    y->parent = x->parent;
    
    if (!x->parent)
        root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
        
    y->left = x;
    x->parent = y;
}

// 右旋操作
void VirtualMemoryTree::right_rotate(VmArea* y) {
    VmArea* x = y->left;
    y->left = x->right;
    
    if (x->right)
        x->right->parent = y;
        
    x->parent = y->parent;
    
    if (!y->parent)
        root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;
        
    x->right = y;
    y->parent = x;
}

// 插入修复
void VirtualMemoryTree::insert_fixup(VmArea* z) {
    while (z->parent && z->parent->color == Color::RED) {
        if (z->parent == z->parent->parent->left) {
            VmArea* y = z->parent->parent->right;
            if (y && y->color == Color::RED) {
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(z);
                }
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                right_rotate(z->parent->parent);
            }
        } else {
            VmArea* y = z->parent->parent->left;
            if (y && y->color == Color::RED) {
                z->parent->color = Color::BLACK;
                y->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(z);
                }
                z->parent->color = Color::BLACK;
                z->parent->parent->color = Color::RED;
                left_rotate(z->parent->parent);
            }
        }
    }
    root->color = Color::BLACK;
}

// 查找最小节点
VmArea* VirtualMemoryTree::minimum(VmArea* node) {
    while (node->left)
        node = node->left;
    return node;
}

// 节点替换
void VirtualMemoryTree::transplant(VmArea* u, VmArea* v) {
    if (!u->parent)
        root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    if (v)
        v->parent = u->parent;
}

// 删除修复
void VirtualMemoryTree::delete_fixup(VmArea* x) {
    while (x != root && (!x || x->color == Color::BLACK)) {
        if (x == x->parent->left) {
            VmArea* w = x->parent->right;
            if (w->color == Color::RED) {
                w->color = Color::BLACK;
                x->parent->color = Color::RED;
                left_rotate(x->parent);
                w = x->parent->right;
            }
            if ((!w->left || w->left->color == Color::BLACK) &&
                (!w->right || w->right->color == Color::BLACK)) {
                w->color = Color::RED;
                x = x->parent;
            } else {
                if (!w->right || w->right->color == Color::BLACK) {
                    if (w->left)
                        w->left->color = Color::BLACK;
                    w->color = Color::RED;
                    right_rotate(w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = Color::BLACK;
                if (w->right)
                    w->right->color = Color::BLACK;
                left_rotate(x->parent);
                x = root;
            }
        } else {
            VmArea* w = x->parent->left;
            if (w->color == Color::RED) {
                w->color = Color::BLACK;
                x->parent->color = Color::RED;
                right_rotate(x->parent);
                w = x->parent->left;
            }
            if ((!w->right || w->right->color == Color::BLACK) &&
                (!w->left || w->left->color == Color::BLACK)) {
                w->color = Color::RED;
                x = x->parent;
            } else {
                if (!w->left || w->left->color == Color::BLACK) {
                    if (w->right)
                        w->right->color = Color::BLACK;
                    w->color = Color::RED;
                    left_rotate(w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = Color::BLACK;
                if (w->left)
                    w->left->color = Color::BLACK;
                right_rotate(x->parent);
                x = root;
            }
        }
    }
    if (x)
        x->color = Color::BLACK;
}

// 查找合适的空闲区域
VmArea* VirtualMemoryTree::find_free_area(uint32_t size) {
    VmArea* best_fit = nullptr;
    VmArea* current = root;
    
    while (current) {
        if (current->size >= size) {
            if (!best_fit || current->size < best_fit->size)
                best_fit = current;
            current = current->left;
        } else {
            current = current->right;
        }
    }
    
    return best_fit;
}

// 分配内存区域
uint32_t VirtualMemoryTree::allocate(uint32_t size) {
    VmArea* area = find_free_area(size);
    if (!area)
        return 0;
    
    uint32_t alloc_addr = area->start_addr;
    
    // 如果区域大小大于请求大小，分割区域
    if (area->size > size) {
        VmArea* new_area = new VmArea(area->start_addr + size,
                                     area->size - size);
        area->size = size;
        
        // 插入新的空闲区域
        new_area->parent = area->parent;
        if (!area->parent)
            root = new_area;
        else if (area == area->parent->left)
            area->parent->left = new_area;
        else
            area->parent->right = new_area;
            
        insert_fixup(new_area);
    }
    
    allocated_size += size;
    return alloc_addr;
}

// 释放内存区域
void VirtualMemoryTree::free(uint32_t addr) {
    VmArea* area = root;
    while (area) {
        if (area->start_addr == addr) {
            allocated_size -= area->size;
            
            // 合并相邻的空闲区域
            VmArea* next = area->right;
            if (next && area->start_addr + area->size == next->start_addr) {
                area->size += next->size;
                transplant(next, next->right);
                delete next;
            }
            
            VmArea* prev = area->left;
            if (prev && prev->start_addr + prev->size == area->start_addr) {
                prev->size += area->size;
                transplant(area, area->right);
                delete area;
            }
            
            return;
        }
        area = (addr < area->start_addr) ? area->left : area->right;
    }
}

// 获取可用内存大小
uint32_t VirtualMemoryTree::get_free_size() const {
    return total_size - allocated_size;
}

// 清理红黑树
void VirtualMemoryTree::cleanup(VmArea* node) {
    if (!node) return;
    cleanup(node->left);
    cleanup(node->right);
    delete node;
}