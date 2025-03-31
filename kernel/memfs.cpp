#include <kernel/cpio.h>
#include <kernel/kernel.h>
#include <kernel/memfs.h>
#include <lib/console.h>
#include <lib/debug.h>
#include <lib/string.h>

namespace kernel
{

MemFSFileDescriptor::MemFSFileDescriptor(MemFSInode* inode) : inode(inode), offset(0) {}

MemFSFileDescriptor::~MemFSFileDescriptor() {}

ssize_t MemFSFileDescriptor::read(void* buffer, size_t size)
{
    debug_debug(
        "MemFSFileDescriptor::read inode: %x, offset:%d, inode->size:%d\n", inode, offset, size);
    if(offset >= inode->size) {
        return 0;
    }
    debug_debug("inode->data %x\n", inode->data);

    size_t remaining = inode->size - offset;
    size_t read_size = size < remaining ? size : remaining;

    debug_debug("read to buffer %x, from:%x, size: %d\n", buffer, inode->data + offset, read_size);
    auto to = buffer;
    auto from = inode->data + offset;

    // auto pde = *(uint32_t*)(0xc0400404);
    // debug_debug("pde: %x", pde);
    // Kernel::instance().kernel_mm().paging().disablePaging();
    // pde = *(uint32_t*)(0x400404);
    // debug_debug("pde: %x", pde);
    // Kernel::instance().kernel_mm().paging().enablePaging();

    for(size_t i = 0; i < read_size; i++) {
        // debug_debug("read %d, %d", i, read_size);
        ((uint8_t*)to)[i] = ((uint8_t*)from)[i];
    }
    debug_debug("read from %x, to:%x, done\n", from, to);
    // memcpy(buffer, inode->data + offset, read_size);
    offset += read_size;
    debug_debug("read_size %x\n", read_size);
    return read_size;
}

ssize_t MemFSFileDescriptor::write(const void* buffer, size_t size)
{
    if(offset + size > inode->capacity) {
        size_t new_capacity = (offset + size) * 2;
        uint8_t* new_data = new uint8_t[new_capacity];

        if(inode->data) {
            memcpy(new_data, inode->data, inode->size);
            delete[] inode->data;
        }

        inode->data = new_data;
        inode->capacity = new_capacity;
    }

    memcpy(inode->data + offset, buffer, size);
    offset += size;
    if(offset > inode->size)
        inode->size = offset;
    return size;
}

int MemFSFileDescriptor::seek(size_t new_offset)
{
    if(new_offset > inode->size)
        return -1;
    offset = new_offset;
    return 0;
}

int MemFSFileDescriptor::close()
{
    delete this;
    return 0;
}

MemFS::MemFS() : root(nullptr) {}

char* MemFS::get_name()
{
    return "memfs";
}

MemFS::~MemFS()
{
    if(root)
        free_inode(root);
}

void MemFS::init()
{
    root = create_inode("", FileType::Directory);
    printk("MemFS initialized with root directory\n");
    debug_info("MemFS root inode created at %x\n", (unsigned int)root);
}

int MemFS::load_initramfs(const void* data, size_t size)
{
    const uint8_t* ptr = static_cast<const uint8_t*>(data);
    const uint8_t* end = ptr + size;

    debug_info("Loading initramfs, data at %x, size: %d bytes\n", (unsigned int)data, size);

    if(size < sizeof(CPIOHeader)) {
        debug_err("Invalid initramfs size: %d bytes (minimum required: %d bytes)\n", size,
            sizeof(CPIOHeader));
        return -1;
    }
    debug_notice("Initramfs size validation passed: %d bytes\n", size);

    while(ptr + sizeof(CPIOHeader) <= end) {
        debug_debug("Processing CPIO header at %x\n", (unsigned int)ptr);
        const CPIOHeader* header = reinterpret_cast<const CPIOHeader*>(ptr);
        //        debug_info(header->magic);
        if(!magic_match(header->magic)) {
            debug_err("Invalid CPIO header magic: %c %c %c\n", header->magic[0], header->magic[1],
                header->magic[2]);
            debug_err(
                "magic[3..6]: %c %c %c\n", header->magic[3], header->magic[4], header->magic[5]);
            //    break;
        }

        ptr += sizeof(CPIOHeader);
        if(ptr >= end) {
            debug_err("Unexpected end of initramfs");
            break;
        }

        // 获取文件名
        uint16_t namesize = get_namesize(header);
        debug_info("namesize: %d\n", namesize);
        if(ptr + namesize > end) {
            debug_err("Unexpected end of initramfs\n");
            break;
        }

        const char* name = reinterpret_cast<const char*>(ptr);
        debug_debug("Found file: %s\n", name);
        // ptr += (namesize +3 )/4*4;
        ptr += namesize;
        ptr = (uint8_t*)(((uintptr_t)ptr + 3) / 4 * 4);

        // 检查是否为结束标记
        if(is_trailer(name)) {
            debug_debug("Trailer found, ending initramfs loading\n");
            break;
        }

        // 获取文件数据
        uint32_t filesize = get_filesize(header);
        debug_debug("filesize: %d\n", filesize);
        if(ptr + filesize > end) {
            debug_debug("Unexpected end of initramfs\n");
            break;
        }

        // 创建文件或目录
        FileType type = is_directory(header) ? FileType::Directory : FileType::Regular;
        MemFSInode* inode = create_inode(name, type);

        debug_debug("Inode created at %x\n", (unsigned int)inode);
        if(type == FileType::Regular && filesize > 0) {
            debug_debug("Creating data buffer for file, size: %d bytes\n", filesize);
            inode->data = new uint8_t[filesize];
            inode->size = filesize;
            inode->capacity = filesize;
            debug_debug(
                "Data buffer created at %x, size: %d bytes\n", (unsigned int)inode->data, filesize);
            memcpy(inode->data, ptr, filesize);
            debug_debug("Data copied, size: %d bytes\n", filesize);
        } else {
            debug_debug("filetype is %d, filesize is %d\n", type, filesize);
            inode->data = nullptr;
            inode->size = 0;
            inode->capacity = 0;
        }

        // 添加到文件系统
        inode->parent = root;
        inode->next = root->children;
        root->children = inode;

        ptr += filesize;
        ptr = (uint8_t*)(((uintptr_t)ptr + 3) / 4 * 4);
    }
    debug_debug("Initramfs loaded successfully, returning 0\n");
    return 0;
}

MemFSInode* MemFS::find_inode(const char* path)
{
    if(!path || !*path) {
        debug_debug("find_inode called with empty path, returning root\n");
        return root;
    }

    debug_debug("Finding inode for path: %s\n", path);
    MemFSInode* current = root;
    char name[256];
    const char* p = path;

    while(*p) {
        // 跳过开头的斜杠
        while(*p == '/')
            p++;
        if(!*p)
            break;

        // 获取下一个路径组件
        const char* end = strchr(p, '/');
        size_t len = 0;
        if(end == nullptr) {
            len = strlen(p);
        } else {
            len = end - p;
        }
        if(len >= sizeof(name))
            return nullptr;

        strncpy(name, p, len);
        name[len] = '\0';

        // 在当前目录中查找
        MemFSInode* found = nullptr;
        for(MemFSInode* child = current->children; child; child = child->next) {
            if(strcmp(child->name, name) == 0) {
                found = child;
                break;
            }
        }

        if(!found)
            return nullptr;
        current = found;
        p += len;
    }

    return current;
}

MemFSInode* MemFS::create_inode(const char* name, FileType type)
{
    debug_debug("Creating new inode, name: %s, type: %d\n", name, (int)type);

    MemFSInode* inode = new MemFSInode;
    strncpy(inode->name, name, sizeof(inode->name) - 1);
    inode->type = type;
    inode->perm = {true, true, true};
    inode->data = nullptr;
    inode->size = 0;
    inode->capacity = 0;
    inode->parent = nullptr;
    inode->children = nullptr;
    inode->next = nullptr;

    debug_debug("Inode created at %x\n", (unsigned int)inode);
    return inode;
}

void MemFS::free_inode(MemFSInode* inode)
{
    if(!inode)
        return;

    // 递归释放子节点
    while(inode->children) {
        MemFSInode* child = inode->children;
        inode->children = child->next;
        free_inode(child);
    }

    // 释放数据和节点本身
    if(inode->data)
        delete[] inode->data;
    delete inode;
}

FileDescriptor* MemFS::open(const char* path)
{
    MemFSInode* inode = find_inode(path);
    if(!inode)
        return nullptr;
    return new MemFSFileDescriptor(inode);
}

int MemFS::stat(const char* path, FileAttribute* attr)
{
    MemFSInode* inode = find_inode(path);
    if(!inode)
        return -1;

    attr->type = inode->type;
    attr->perm = inode->perm;
    attr->size = inode->size;
    return 0;
}

int MemFS::mkdir(const char* path)
{
    // 查找父目录
    const char* last_slash = strrchr(path, '/');
    if(!last_slash)
        return -1;

    char parent_path[256];
    size_t parent_len = last_slash - path;
    if(parent_len >= sizeof(parent_path))
        return -1;

    strncpy(parent_path, path, parent_len);
    parent_path[parent_len] = '\0';

    MemFSInode* parent = find_inode(parent_path);
    if(!parent || parent->type != FileType::Directory)
        return -1;

    // 获取目录名
    const char* name = last_slash + 1;
    if(!*name)
        return -1;

    // 检查目录是否已存在
    for(MemFSInode* child = parent->children; child; child = child->next) {
        if(strcmp(child->name, name) == 0)
            return -1;
    }

    // 创建新目录
    MemFSInode* dir = create_inode(name, FileType::Directory);
    dir->parent = parent;
    dir->next = parent->children;
    parent->children = dir;

    return 0;
}

int MemFS::unlink(const char* path)
{
    if(!path || !*path)
        return -1;

    MemFSInode* file = find_inode(path);
    if(!file || file->type != FileType::Regular)
        return -1;

    // 从父目录中移除
    MemFSInode* parent = file->parent;
    if(!parent)
        return -1;

    if(parent->children == file) {
        parent->children = file->next;
    } else {
        for(MemFSInode* prev = parent->children; prev; prev = prev->next) {
            if(prev->next == file) {
                prev->next = file->next;
                break;
            }
        }
    }

    // 释放文件节点
    free_inode(file);
    return 0;
}

int MemFS::rmdir(const char* path)
{
    if(!path || !*path || strcmp(path, "/") == 0)
        return -1;

    MemFSInode* dir = find_inode(path);
    if(!dir || dir->type != FileType::Directory)
        return -1;

    // 检查目录是否为空
    if(dir->children)
        return -1;

    // 从父目录中移除
    MemFSInode* parent = dir->parent;
    if(!parent)
        return -1;

    if(parent->children == dir) {
        parent->children = dir->next;
    } else {
        for(MemFSInode* prev = parent->children; prev; prev = prev->next) {
            if(prev->next == dir) {
                prev->next = dir->next;
                break;
            }
        }
    }

    // 释放目录节点
    free_inode(dir);
    return 0;
}

} // namespace kernel