#include "kernel/fs/PageCache.h"
#include <drivers/block_device.h>
#include <kernel/fs/SimplePageCache.h>
#include <lib/string.h>

SimplePageCache::SimplePageCache(kernel::BlockDevice* dev, size_t page_size, size_t max_pages)
    : dev_(dev), page_size_(page_size), max_pages_(max_pages)
{
}
SimplePageCache::~SimplePageCache() { clear(); }

bool SimplePageCache::exists(const PageKey& key) const
{
    return cache_.find(key) != nullptr;
}

Page* SimplePageCache::get_page(const PageKey& key)
{
    kernel::LockGuard lock(mtx_);
    auto it = cache_.find(key);
    if (it) {
        return it;
    }

    // 从设备读取页面
    if(cache_.size() >= max_pages_) {
        // 淘汰策略：移除最早插入的页
        // TODO:
        // cache_.erase(cache_.begin());
    }
    Page page{};
    page.size = page_size_;
    page.data = new uint8_t[page_size_];
    dev_->read_block(key.block_id, page.data);
    page.dirty = false;
    auto ret = cache_.insert(key, page);
    return ret;
}

size_t SimplePageCache::read_page(
    const PageKey& key, size_t offset, void* buf, size_t size)
{
    kernel::LockGuard lock(mtx_);
    auto it = cache_.find(key);
    if (!it) {
        return 0;
    }
    if(offset >= page_size_)
        return 0;
    size_t n = min(size, page_size_ - offset);
    memcpy(buf, static_cast<uint8_t*>(it->data) + offset, n);
    return n;
}

size_t SimplePageCache::write_page(
    const PageKey& key, size_t offset, const void* buf, size_t size)
{
    kernel::LockGuard lock(mtx_);
    auto it = cache_.find(key);
    if (!it) {
        return 0;
    }
    if(offset >= page_size_)
        return 0;
    size_t n = min(size, page_size_ - offset);
    memcpy(static_cast<uint8_t*>(it->data) + offset, buf, n);
    it->dirty = true;
    return n;
}

void SimplePageCache::mark_dirty(const PageKey& key)
{
    kernel::LockGuard lock(mtx_);
    auto it = cache_.find(key);
    if(it) {
        it->dirty = true;
    }
}

bool SimplePageCache::flush(const PageKey& key)
{
    kernel::LockGuard lock(mtx_);
    auto it = cache_.find(key);
    if(!it) {
        return false;
    }
    if(!it->dirty)
        return true;
    // TODO: 实际写回到后端存储，这里仅标记为已刷写
    it->dirty = false;
    return true;
}

void SimplePageCache::flush_all()
{
    kernel::LockGuard lock(mtx_);
    cache_.for_each([](const PageKey& key, Page& page) {
        if(page.dirty) {
            // TODO: 实际写回到后端存储
            page.dirty = false;
        }
    });
}

void SimplePageCache::invalidate(const PageKey& key)
{
    kernel::LockGuard lock(mtx_);
    auto it = cache_.find(key);
    if(it != nullptr) {
        delete[] static_cast<uint8_t*>(it->data);
        cache_.erase(key);
    }
}

void SimplePageCache::clear()
{
    kernel::LockGuard lock(mtx_);
    cache_.for_each([this](const PageKey& key, Page& page) {
        delete[] static_cast<uint8_t*>(page.data);
    });
    cache_.clear();
}

size_t SimplePageCache::page_count() const
{
    kernel::LockGuard lock(mtx_);
    return cache_.size();
}

void SimplePageCache::set_max_pages(size_t max_pages)
{
    kernel::LockGuard lock(mtx_);
    max_pages_ = max_pages;
    while(cache_.size() > max_pages_) {
        //cache_.erase(cache_.begin());
        // TODO:
    }
}

HashList::HashList(size_t bucket_count)
    : bucket_count_(bucket_count), size_(0)
{
    buckets = new HashListNode*[bucket_count_]();
}

HashList::~HashList() {
    clear();
    delete[] buckets;
}

size_t HashList::hash(const PageKey& key) const {
    // 简单hash：直接用block_id
    return (size_t)(key.block_id) % bucket_count_;
}

Page* HashList::find(const PageKey& key) {
    HashListNode* node = buckets[hash(key)];
    while (node) {
        if (node->key == key) return &node->value;
        node = node->next;
    }
    return nullptr;
}

const Page* HashList::find(const PageKey& key) const {
    HashListNode* node = buckets[hash(key)];
    while (node) {
        if (node->key == key) return &node->value;
        node = node->next;
    }
    return nullptr;
}

Page* HashList::insert(const PageKey& key, const Page& value) {
    size_t idx = hash(key);
    HashListNode* node = buckets[idx];
    while (node) {
        if (node->key == key) {
            node->value = value; // 覆盖
            return &node->value;
        }
        node = node->next;
    }
    // 新插入
    node = new HashListNode{key, value, buckets[idx]};
    buckets[idx] = node;
    ++size_;
    return &node->value;
}

bool HashList::erase(const PageKey& key) {
    size_t idx = hash(key);
    HashListNode** pnode = &buckets[idx];
    while (*pnode) {
        if ((*pnode)->key == key) {
            HashListNode* to_delete = *pnode;
            *pnode = to_delete->next;
            delete to_delete;
            --size_;
            return true;
        }
        pnode = &((*pnode)->next);
    }
    return false;
}

void HashList::clear() {
    for (size_t i = 0; i < bucket_count_; ++i) {
        HashListNode* node = buckets[i];
        while (node) {
            HashListNode* next = node->next;
            delete node;
            node = next;
        }
        buckets[i] = nullptr;
    }
    size_ = 0;
}

size_t HashList::size() const { return size_; }

template<typename Func>
void HashList::for_each(Func f) {
    for (size_t i = 0; i < bucket_count_; ++i) {
        HashListNode* node = buckets[i];
        while (node) {
            f(node->key, node->value);
            node = node->next;
        }
    }
}
