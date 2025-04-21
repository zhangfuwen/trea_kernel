#pragma once
#include <cstddef>
#include <cstdint>

// 页标识，可根据你的需求扩展
struct PageKey {
    uint64_t block_id;   // 页号
    bool operator==(const PageKey& other) const {
        return block_id == other.block_id;
    }
    // 可加hash函数用于unordered_map
};

// 单页抽象
struct Page {
    void* data;         // 指向实际页缓冲区
    size_t size;        // 页大小
    bool dirty;         // 脏页标志
    // 可扩展引用计数、锁、时间戳等
};

class PageCache {
public:
    virtual ~PageCache() = default;

    virtual bool exists(const PageKey& key) const = 0;
    // 获取页指针，create为true时若不存在则分配新页
    // 返回nullptr表示失败
    virtual Page* get_page(const PageKey& key) = 0;

    // 读页内容（页内偏移+长度），返回实际读取字节数
    virtual size_t read_page(const PageKey& key, size_t offset, void* buf, size_t size) = 0;

    // 写页内容（页内偏移+长度），自动mark dirty，返回实际写入字节数
    virtual size_t write_page(const PageKey& key, size_t offset, const void* buf, size_t size) = 0;

    // 标记脏页
    virtual void mark_dirty(const PageKey& key) = 0;

    // 刷新单页（写回后端），返回是否成功
    virtual bool flush(const PageKey& key) = 0;

    // 刷新所有脏页
    virtual void flush_all() = 0;

    // 失效（移除）某页，必要时可先flush
    virtual void invalidate(const PageKey& key) = 0;

    // 清空所有缓存（必要时flush所有脏页）
    virtual void clear() = 0;

    // 获取缓存中页的数量
    virtual size_t page_count() const = 0;

    // 设置最大缓存页数
    virtual void set_max_pages(size_t max_pages) = 0;

    // 线程安全：建议所有接口内部加锁，或由调用方保证
};
// 简单哈希链表，适用于PageKey->Page映射
struct HashListNode {
    PageKey key;
    Page value;
    HashListNode* next;
};

class HashList {
public:
    explicit HashList(size_t bucket_count = 256);
    ~HashList();

    Page* find(const PageKey& key);
    const Page* find(const PageKey& key) const;
    Page* insert(const PageKey& key, const Page& value); // 若已存在则覆盖
    bool erase(const PageKey& key);
    void clear();
    size_t size() const;

    // 遍历所有元素
    template<typename Func>
    void for_each(Func f);

private:
    size_t bucket_count_;
    size_t size_;
    HashListNode** buckets;

    size_t hash(const PageKey& key) const;
};