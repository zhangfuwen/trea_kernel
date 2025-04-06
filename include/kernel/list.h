#pragma once

namespace kernel {

// 双向链表节点结构
struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

// 初始化链表头
inline void INIT_LIST_HEAD(struct list_head *list) {
    list->next = list;
    list->prev = list;
}

// 在两个节点之间插入新节点
inline void __list_add(struct list_head *new_node,
                      struct list_head *prev,
                      struct list_head *next) {
    next->prev = new_node;
    new_node->next = next;
    new_node->prev = prev;
    prev->next = new_node;
}

// 在指定节点之后插入新节点
inline void list_add(struct list_head *new_node, struct list_head *head) {
    __list_add(new_node, head, head->next);
}

// 在指定节点之前插入新节点（即在链表尾部添加）
inline void list_add_tail(struct list_head *new_node, struct list_head *head) {
    __list_add(new_node, head->prev, head);
}

// 删除两个节点之间的节点
inline void __list_del(struct list_head *prev, struct list_head *next) {
    next->prev = prev;
    prev->next = next;
}

// 删除指定节点并重新初始化它
inline void list_del_init(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
    INIT_LIST_HEAD(entry);
}

// 判断链表是否为空
inline bool list_empty(const struct list_head *head) {
    return head->next == head;
}

// 从成员指针获取结构体指针的宏
#define container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))

// 从链表节点获取包含该节点的结构体指针
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

} // namespace kernel