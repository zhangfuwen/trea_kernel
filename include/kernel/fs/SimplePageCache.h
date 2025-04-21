#pragma once
#include "../../lib/mutex.h"
#include "kernel/fs/PageCache.h"

class SimplePageCache : public PageCache {
public:
    SimplePageCache(kernel::BlockDevice *dev, size_t page_size, size_t max_pages);
    ~SimplePageCache() override;

    bool exists(const PageKey& key) const override;
    Page* get_page(const PageKey& key) override;
    size_t read_page(const PageKey& key, size_t offset, void* buf, size_t size) override;
    size_t write_page(const PageKey& key, size_t offset, const void* buf, size_t size) override;
    void mark_dirty(const PageKey& key) override;
    bool flush(const PageKey& key) override;
    void flush_all() override;
    void invalidate(const PageKey& key) override;
    void clear() override;
    size_t page_count() const override;
    void set_max_pages(size_t max_pages) override;

private:
    size_t page_size_;
    size_t max_pages_;
    kernel::BlockDevice *dev_;
    mutable kernel::Mutex mtx_;
    HashList cache_;
};

