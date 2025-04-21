#include "drivers/block_device.h"
#include <drivers/ext2.h>
#include <kernel/dirent.h>
#include <lib/debug.h>
#include <lib/string.h>
#include "kernel/fs/SimplePageCache.h"

namespace kernel
{

Ext2FileSystem::Ext2FileSystem(BlockDevice* device) : device(device)
{
    log_debug("[ext2] 初始化文件系统 device:%p\n", device);
    super_block = new Ext2SuperBlock();
    page_cache = new SimplePageCache(device,4096, 1024);
    auto ret = read_super_block();
    log_debug("read_super_block ret %d, super block:0x%x\n", ret, super_block);
    if(ret) {
        hexdump(super_block, sizeof(Ext2SuperBlock),
            [](const char* line) { log_debug("%s\n", line); });
        super_block->print();
    }
    // if(!ret) {
    //     // 初始化新的文件系统
    //     // super_block->inodes_count = 1024;
    //     // super_block->blocks_count = device->get_info().total_blocks;
    //     // super_block->first_data_block = 1;
    //     // super_block->block_size = device->get_info().block_size;
    //     // super_block->blocks_per_group = 8192;
    //     // super_block->inodes_per_group = 1024;
    //     // super_block->magic = EXT2_MAGIC;
    //     // super_block->state = 1; // 文件系统正常
    //     //
    //     // // 写入超级块
    //     // device->write_block(0, super_block);
    //     //
    //     // // 初始化根目录
    //     // auto root_inode = new Ext2Inode();
    //     // root_inode->mode = 0x4000; // 目录类型
    //     // root_inode->size = 0;
    //     // root_inode->blocks = 0;
    //     // write_inode(1, root_inode);
    //     // delete root_inode;
    // }
}

Ext2FileSystem::~Ext2FileSystem()
{
    if(super_block) {
        delete super_block;
    }
}

char* Ext2FileSystem::get_name() { return "ext2"; }

// 打印 Ext2GroupDesc 结构体的成员，包含十进制和十六进制值
void Ext2GroupDesc::print()
{
    auto& group_desc = *(Ext2GroupDesc*)this;
    log_debug("Ext2 Group Descriptor:\n");
    log_debug("  bg_block_bitmap:      %d (0x%x)\n", group_desc.bg_block_bitmap,
        group_desc.bg_block_bitmap);
    log_debug("  bg_inode_bitmap:      %d (0x%x)\n", group_desc.bg_inode_bitmap,
        group_desc.bg_inode_bitmap);
    log_debug("  bg_inode_table:       %d (0x%x)\n", group_desc.bg_inode_table,
        group_desc.bg_inode_table);
    log_debug("  bg_free_blocks_count: %d (0x%x)\n", group_desc.bg_free_blocks_count,
        group_desc.bg_free_blocks_count);
    log_debug("  bg_free_inodes_count: %d (0x%x)\n", group_desc.bg_free_inodes_count,
        group_desc.bg_free_inodes_count);
    log_debug("  bg_used_dirs_count:   %d (0x%x)\n", group_desc.bg_used_dirs_count,
        group_desc.bg_used_dirs_count);
    log_debug("  bg_pad:               %d (0x%x)\n", group_desc.bg_pad, group_desc.bg_pad);
    for(int i = 0; i < 3; ++i) {
        log_debug("  bg_reserved[%d]:    %d (0x%x)\n", i, group_desc.bg_reserved[i],
            group_desc.bg_reserved[i]);
    }
}
void Ext2Inode::print()
{
    log_debug("Ext2Inode:\n");
    log_debug("  mode: 0x%04x (%d)\n", mode, mode);
    log_debug("  uid: %u (0x%x)\n", uid, uid);
    log_debug("  size: %u (0x%x)\n", size, size);
    log_debug("  atime: %u (0x%x)\n", atime, atime);
    log_debug("  ctime: %u (0x%x)\n", ctime, ctime);
    log_debug("  mtime: %u (0x%x)\n", mtime, mtime);
    log_debug("  dtime: %u (0x%x)\n", dtime, dtime);
    log_debug("  gid: %u (0x%x)\n", gid, gid);
    log_debug("  i_links_count: %u (0x%x)\n", i_links_count, i_links_count);
    log_debug("  i_blocks: %u (0x%x)\n", blocks, blocks);
    log_debug("  flags: %u (0x%x)\n", flags, flags);
    log_debug("  osd1: %u (0x%x)\n", osd1, osd1);
    log_debug("  i_generation: %u (0x%x)\n", i_generation, i_generation);
    log_debug("  i_file_acl: %u (0x%x)\n", i_file_acl, i_file_acl);
    log_debug("  i_dir_acl: %u (0x%x)\n", i_dir_acl, i_dir_acl);
    log_debug("  i_faddr: %u (0x%x)\n", i_faddr, i_faddr);
    log_debug("  osd2: {\n");
    log_debug("    l_i_frag: %u (0x%x)\n", osd2.l_i_frag, osd2.l_i_frag);
    log_debug("    l_i_fsize: %u (0x%x)\n", osd2.l_i_fsize, osd2.l_i_fsize);
    log_debug("    i_pad1: %u (0x%x)\n", osd2.i_pad1, osd2.i_pad1);
    log_debug("    l_i_uid_high: %u (0x%x)\n", osd2.l_i_uid_high, osd2.l_i_uid_high);
    log_debug("    l_i_gid_high: %u (0x%x)\n", osd2.l_i_gid_high, osd2.l_i_gid_high);
    log_debug("    l_i_reserved2: %u (0x%x)\n", osd2.l_i_reserved2, osd2.l_i_reserved2);
    log_debug("  }\n");
    for(int i = 0; i < 15; ++i) {
        log_debug("  i_block[%d]: %d\n", i, i_block[i]);
    }
}

void Ext2SuperBlock::print()
{
    log_debug("Ext2SuperBlock:\n");
    log_debug("  inodes_count: %d (0x%x)\n", inodes_count, inodes_count);
    log_debug("  blocks_count: %d (0x%x)\n", blocks_count, blocks_count);
    log_debug("  r_blocks_count: %d (0x%x)\n", r_blocks_count, r_blocks_count);
    log_debug("  free_blocks_count: %d (0x%x)\n", free_blocks_count, free_blocks_count);
    log_debug("  free_inodes_count: %d (0x%x)\n", free_inodes_count, free_inodes_count);
    log_debug("  first_data_block: %d (0x%x)\n", first_data_block, first_data_block);
    log_debug("  log_block_size: %d (0x%x)\n", log_block_size, log_block_size);
    log_debug("  blocks_per_group: %d (0x%x)\n", blocks_per_group, blocks_per_group);
    log_debug("  frags_per_group: %d (0x%x)\n", frags_per_group, frags_per_group);
    log_debug("  inodes_per_group: %d (0x%x)\n", inodes_per_group, inodes_per_group);
    log_debug("  mtime: %d (0x%x)\n", mtime, mtime);
    log_debug("  wtime: %d (0x%x)\n", wtime, wtime);
    log_debug("  mnt_count: %d (0x%x)\n", mnt_count, mnt_count);
    log_debug("  max_mnt_count: %d (0x%x)\n", max_mnt_count, max_mnt_count);
    log_debug("  magic: 0x%x\n", magic);
    log_debug("  state: %d (0x%x)\n", state, state);
    log_debug("  errors: %d (0x%x)\n", errors, errors);
    log_debug("  minor_rev_level: %d (0x%x)\n", minor_rev_level, minor_rev_level);
    log_debug("  lastcheck: %d (0x%x)\n", lastcheck, lastcheck);
    log_debug("  checkinterval: %d (0x%x)\n", checkinterval, checkinterval);
    log_debug("  creator_os: %d (0x%x)\n", creator_os, creator_os);
    log_debug("  rev_level: %d (0x%x)\n", rev_level, rev_level);
    log_debug("  def_resuid: %d (0x%x)\n", def_resuid, def_resuid);
    log_debug("  def_resgid: %d (0x%x)\n", def_resgid, def_resgid);
    log_debug("  first_ino: %d (0x%x)\n", first_ino, first_ino);
    log_debug("  inode_size: %d (0x%x)\n", inode_size, inode_size);
    log_debug("  block_group_nr: %d (0x%x)\n", block_group_nr, block_group_nr);
}

bool Ext2FileSystem::read_super_block()
{
    // 读取超级块
    auto block_size = 4096;
    auto super_block_offset = 1024;
    auto buffer = new uint8_t[block_size];
    auto ret = device->read_block(super_block_offset / block_size, buffer);
    if(!ret) {
        log_err("read_super_block failed\n");
        return false;
    }

    auto copy_offset = super_block_offset % block_size;
    memcpy(super_block, buffer + copy_offset, sizeof(Ext2SuperBlock));
    log_debug("read_super_block buffer, copy_offset:%d(0x%x):\n", copy_offset, copy_offset);
    hexdump(buffer + copy_offset, sizeof(*super_block),
        [](const char* line) { log_debug("%s\n", line); });
    delete[] buffer;

    if(super_block->magic != EXT2_MAGIC) {
        log_err("read_super_block failed, magic error:0x%x, expect:0x%x\n", super_block->magic,
            EXT2_MAGIC);
        return false;
    }

    // debug_debug("ext4 block size:%d(0x%x)\n", super_block->block_size(),
    // super_block->block_size()); debug_debug("device block size:%d(0x%x)\n",
    // device->get_info().block_size, device->get_info().block_size); uint32_t block_size =
    // super_block->block_size(); auto num_sectors_per_block = block_size /
    // device->get_info().block_size; debug_debug("num_sectors_per_block: %d(0x%x)\n",
    // num_sectors_per_block, num_sectors_per_block);

    // 读取块组描述符表（位于超级块下一个块）
    uint32_t group_desc_block = super_block->first_data_block + 1;
    uint8_t* group_desc_buffer = new uint8_t[block_size];

    log_debug("group_desc_block:%d(0x%x)\n", group_desc_block, group_desc_block);
    if(!device->read_block(group_desc_block, group_desc_buffer)) {
        log_err("读取块组描述符表失败 block:%u\n", group_desc_block);
        delete[] group_desc_buffer;
        return false;
    }

    // 解析第一个块组描述符
    memcpy(&group_desc, group_desc_buffer, sizeof(Ext2GroupDesc));
    delete[] group_desc_buffer;

    log_debug("块组描述符 bg_inode_table=%u\n", group_desc.bg_inode_table);
    group_desc.print();

    // 读取根目录inode（inode号2）
    uint32_t inode_table_block = group_desc.bg_inode_table;
    uint32_t inodes_per_block = block_size / super_block->inode_size;
    uint32_t root_inode_offset = (EXT2_ROOT_INO - 1) % inodes_per_block * super_block->inode_size;

    uint8_t* inode_table_buffer = new uint8_t[block_size];
    if(!device->read_block(inode_table_block, inode_table_buffer)) {
        log_err("读取inode表块失败 block:%u\n", inode_table_block);
        delete[] inode_table_buffer;
        return false;
    }

    memcpy(&root_inodes, inode_table_buffer + root_inode_offset, sizeof(Ext2Inode));
    delete[] inode_table_buffer;

    log_debug("根目录inode信息：\n");
    root_inodes.print();

    return true;
}

uint32_t Ext2Inode::inode_table_block() { return 4; }

Ext2Inode* Ext2FileSystem::read_inode(uint32_t inode_num)
{
    // debug_debug("ext4 block size:%d(0x%x)\n", super_block->block_size(),
    // super_block->block_size()); debug_debug("device block size:%d(0x%x)\n",
    // device->get_info().block_size, device->get_info().block_size);
    uint32_t block_size = super_block->block_size();
    // auto num_sectors_per_block = block_size / device->get_info().block_size;
    // debug_debug("num_sectors_per_block: %d(0x%x)\n", num_sectors_per_block,
    // num_sectors_per_block);

    log_debug("[ext2] 读取inode %u (总数:%u)\n", inode_num, super_block->inodes_count);
    if(inode_num == 0 || inode_num > super_block->inodes_count) {
        log_err("无效的inode编号 %u (范围1-%u)\n", inode_num, super_block->inodes_count);
        return nullptr;
    }

    uint32_t inodes_per_block = block_size / super_block->inode_size;
    uint32_t block_num = (inode_num - 1) / inodes_per_block + group_desc.bg_inode_table;
    uint32_t offset = (inode_num - 1) % inodes_per_block;
    log_debug("计算块号:%u 偏移:%u (块大小:%u inode大小:%u)\n", block_num, offset, block_size,
        super_block->inode_size);

    // block_num = block_num*num_sectors_per_block + offset/device->get_info().block_size;
    // offset = offset % device->get_info().block_size;
    auto block_buffer = new uint8_t[super_block->block_size()];

    if(!device->read_block(block_num, block_buffer)) {
        log_err("读取块%u失败\n", block_num);
        delete[] block_buffer;
        return nullptr;
    }

    auto inode = new Ext2Inode();
    memcpy(inode, block_buffer + offset * super_block->inode_size, sizeof(Ext2Inode));
    // inode->print();
    log_debug("读取inode %u完成 mode:0x%x size:%u blocks:%u\n", inode_num, inode->mode,
        inode->size, inode->blocks);

    delete[] block_buffer;
    return inode;
}

bool Ext2FileSystem::write_inode(uint32_t inode_num, const Ext2Inode* inode)
{
    log_debug("[ext2] 写入inode %u\n", inode_num);
    if(inode_num == 0 || inode_num > super_block->inodes_count || !inode) {
        log_err("无效参数 inode_num:%u ptr:%p\n", inode_num, inode);
        return false;
    }

    uint32_t block_size = device->get_info().sector_size;
    uint32_t inodes_per_block = block_size / sizeof(Ext2Inode);
    uint32_t block_num = (inode_num - 1) / inodes_per_block + super_block->first_data_block;
    uint32_t offset = (inode_num - 1) % inodes_per_block;
    log_debug("写入块号:%u 偏移:%u\n", block_num, offset);

    auto block_buffer = new uint8_t[block_size];
    if(!device->read_block(block_num, block_buffer)) {
        log_err("读取目标块%u失败\n", block_num);
        delete[] block_buffer;
        return false;
    }

    memcpy(block_buffer + offset * sizeof(Ext2Inode), inode, sizeof(Ext2Inode));
    bool result = device->write_block(block_num, block_buffer);
    log_debug("写入inode %u结果:%s\n", inode_num, result ? "成功" : "失败");

    delete[] block_buffer;
    return result;
}

uint32_t Ext2FileSystem::allocate_block()
{
    log_debug("[ext2] 开始分配数据块 (first_data_block:%u blocks_count:%u)\n",
        super_block->first_data_block, super_block->blocks_count);
    auto block_buffer = new uint8_t[device->get_info().sector_size];
    for(uint32_t i = super_block->first_data_block; i < super_block->blocks_count; i++) {
        log_debug("检查块%u...", i);
        if(device->read_block(i, block_buffer)) {
            bool is_free = true;
            for(size_t j = 0; j < device->get_info().sector_size; j++) {
                if(block_buffer[j] != 0) {
                    is_free = false;
                    break;
                }
            }
            log_debug(is_free ? "空闲块发现!\n" : "已占用\n");
            if(is_free) {
                delete[] block_buffer;
                log_debug("成功分配块%u\n", i);
                return i;
            }
        }
    }
    delete[] block_buffer;
    log_err("没有可用数据块!\n");
    return 0;
}

int Ext2FileSystem::mkdir(const char* path)
{
    log_debug("[ext2] 创建目录 %s (当前目录inode:%u)\n", path, current_dir_inode);
    uint32_t new_inode = allocate_inode();
    log_debug("分配的新inode:%u\n", new_inode);
    if(!new_inode) {
        log_err("分配inode失败\n");
        return -1;
    }

    // 初始化目录inode
    auto block_size = super_block->block_size();
    Ext2Inode dir_inode{};
    dir_inode.mode = 0x41FF; // 目录权限
    dir_inode.size = block_size;
    dir_inode.blocks = 1;
    dir_inode.i_block[0] = allocate_block(); // 使用正确的块指针字段

    write_inode(new_inode, &dir_inode);

    // 创建目录条目
    uint8_t* block = new uint8_t[block_size];
    Ext2DirEntry* self = (Ext2DirEntry*)block;
    self->inode = new_inode;
    self->name_len = 1;
    self->rec_len = 12;
    memcpy(self->name, ".", 1);

    Ext2DirEntry* parent = (Ext2DirEntry*)(block + 12);
    parent->inode = current_dir_inode; // 使用当前目录上下文
    parent->name_len = 2;
    parent->rec_len = block_size - 12;
    memcpy(parent->name, "..", 2);

    device->write_block(dir_inode.i_block[0], block); // 修改此处
    delete[] block;

    // 添加条目到父目录
    // ... 需要实现目录条目添加逻辑 ...
    return 0;
}

uint32_t Ext2FileSystem::allocate_inode()
{
    // 简单的inode分配策略：返回第一个可用的inode号
    for(uint32_t i = 1; i <= super_block->inodes_count; i++) {
        auto inode = read_inode(i);
        if(inode && inode->mode == 0) {
            delete inode;
            return i;
        }
        delete inode;
    }
    return 0;
}

// 块/节点释放函数
void free_block(uint32_t block_num)
{
    // 实现块释放逻辑
}

void free_inode(uint32_t inode_num)
{
    // 实现inode释放逻辑
}

FileDescriptor* Ext2FileSystem::open(const char* path)
{
    log_debug("open %s\n", path);
    // 从根目录开始查找（inode 2）
    uint32_t current_inode = EXT2_ROOT_INO;

    // 分割路径
    char* parts = strdup(path);
    char* token = strtok(parts, "/");

    while(token) {
        Ext2Inode* inode = read_inode(current_inode);
        log_debug("inode %d(0x%x)\n", current_inode, current_inode);
        inode->print();
        if(!inode || (inode->mode & 0xF000) != 0x4000) {
            delete inode;
            delete[] parts;
            return nullptr; // 不是目录
        }

        // 读取目录块
        auto block_size = super_block->block_size();
        uint8_t* block = new uint8_t[block_size];
        bool found = false;
        for(uint32_t i = 0; i < inode->blocks; i++) {
            log_debug("reading block %d, %d:\n",i, inode->i_block[i]);
            device->read_block(inode->i_block[i], block);

            Ext2DirEntry* entry = (Ext2DirEntry*)block;
            while((uint8_t*)entry < block + block_size) {
                log_debug("entry inode:%u name:%s\n", entry->inode, entry->name);
                if(entry->inode && entry->name_len == strlen(token) &&
                    memcmp(entry->name, token, entry->name_len) == 0) {
                    current_inode = entry->inode;
                    if(entry->file_type == (unsigned char)FT_DIR) {
                        current_dir_inode = current_inode;
                    }
                    found = true;
                    break;
                }
                entry = (Ext2DirEntry*)((uint8_t*)entry + entry->rec_len);
            }
            if(found)
                break;
        }

        delete[] block;
        delete inode;
        if(!found) {
            delete[] parts;
            return nullptr;
        }
        token = strtok(nullptr, "/");
    }

    delete[] parts;
    return new Ext2FileDescriptor(current_inode, this);
}

int Ext2FileSystem::stat(const char* path, FileAttribute* attr)
{
    log_debug("Ext2FileSystem::stat path:%s\n", path);
    Ext2FileDescriptor* fd = (Ext2FileDescriptor*)open(path);
    if(!fd) {
        log_debug("Ext2FileSystem::open failed\n");
        return -1;
    }

    Ext2Inode* inode = read_inode(fd->m_inode);
    attr->size = inode->size;
    attr->type = (inode->mode & 0xF000) == 0x4000 ? FT_DIR : FT_REG;
    log_debug("inode->mode:0x%x\n", inode->mode);
    delete inode;
    delete fd;
    return 0;
}

// // 修改目录创建代码中的父目录引用
// int Ext2FileSystem::mkdir(const char* path)
// {
//     // 创建目录inode
//     uint32_t new_inode = allocate_inode();
//     if(!new_inode)
//         return -1;
//
//     // 初始化目录inode
//     auto block_size = super_block->block_size;
//     Ext2Inode dir_inode{};
//     dir_inode.mode = 0x41FF; // 目录权限
//     dir_inode.size = block_size;
//     dir_inode.i_blocks = 1;
//     dir_inode.i_block[0] = allocate_block(); // 使用正确的块指针字段
//
//     write_inode(new_inode, &dir_inode);
//
//     // 创建目录条目
//     uint8_t* block = new uint8_t[block_size];
//     Ext2DirEntry* self = (Ext2DirEntry*)block;
//     self->inode = new_inode;
//     self->name_len = 1;
//     self->rec_len = 12;
//     memcpy(self->name, ".", 1);
//
//     Ext2DirEntry* parent = (Ext2DirEntry*)(block + 12);
//     parent->inode = current_dir_inode; // 使用当前目录上下文
//     parent->name_len = 2;
//     parent->rec_len = block_size - 12;
//     memcpy(parent->name, "..", 2);
//
//     device->write_block(dir_inode.i_block[0], block); // 修改此处
//     delete[] block;
//
//     // 添加条目到父目录
//     // ... 需要实现目录条目添加逻辑 ...
//     return 0;
// }

int Ext2FileSystem::unlink(const char* path)
{
    // TODO: 实现文件删除
    Ext2FileDescriptor* fd = (Ext2FileDescriptor*)open(path);
    if(!fd)
        return -1;

    Ext2Inode* inode = read_inode(fd->m_inode);
    inode->i_links_count--; // 修正字段名
    write_inode(fd->m_inode, inode);

    if(inode->i_links_count == 0) {
        // 释放数据块
        for(uint32_t i = 0; i < inode->blocks; i++) { // 修正字段名
            free_block(inode->i_block[i]);            // 修正字段名
        }
        free_inode(fd->m_inode);
    }

    delete inode;
    delete fd;
    return 0;
}

int Ext2FileSystem::rmdir(const char* path)
{
    // TODO: 实现目录删除
    // 需要额外检查目录是否为空
    return unlink(path); // 暂时共用unlink逻辑
}

// 实现文件描述符方法
Ext2FileDescriptor::Ext2FileDescriptor(uint32_t inode, Ext2FileSystem* fs)
    : m_inode(inode), m_position(0), m_fs(fs)
{
}

Ext2FileDescriptor::~Ext2FileDescriptor() {}

int Ext2FileDescriptor::seek(size_t offset)
{
    // const int SEEK_SET = 0;
    // const int SEEK_CUR = 1;
    // const int SEEK_END = 2;
    Ext2Inode* inode = m_fs->read_inode(m_inode);
    if(!inode)
        return 0;
    off_t new_pos = m_position;
    auto whence = SEEK_SET;
    switch(whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos += offset;
            break;
        case SEEK_END:
            new_pos = inode->size + offset;
            break;
        default:
            delete inode;
            return -1;
    }

    if(new_pos < 0 || new_pos > inode->size) {
        delete inode;
        return -1;
    }

    m_position = new_pos;
    delete inode;
    return m_position;
}

int Ext2FileDescriptor::close()
{
    // 释放相关资源
    // return m_fs->close(this);
    return 0;
}

int Ext2FileDescriptor::iterate(void* buffer, size_t buffer_size, uint32_t* pos)
{
    log_debug("enter, buffer:%x, size:%d, pos:0x%x\n", buffer, buffer_size, *pos);
    // 检查是否为目录
    Ext2Inode* inode = m_fs->read_inode(m_inode);
    if(!inode || (inode->mode & 0xF000) != 0x4000) {
        delete inode;
        return -1; // 不是目录
    }

    // 准备缓冲区
    uint8_t* output_buffer = static_cast<uint8_t*>(buffer);
    size_t bytes_written = 0;

    // 读取目录块
    auto block_size = m_fs->super_block->block_size();
    uint8_t* block = new uint8_t[block_size];

    // 计算当前位置对应的块索引和块内偏移
    uint32_t current_pos = *pos;
    uint32_t block_idx = current_pos / block_size;
    uint32_t block_offset = current_pos % block_size;

    log_debug("inode->blocks: %d(0x%x), block_idx:%d, current_pos:0x%x\n", inode->blocks,
        inode->blocks, block_idx, current_pos);

    // 遍历目录中的块，从当前位置开始
    for(uint32_t i = block_idx; i < inode->blocks && bytes_written < buffer_size; i++) {
        log_debug("read block %d(0x%x)\n", inode->i_block[i], inode->i_block[i]);
        if(!m_fs->device->read_block(inode->i_block[i], block)) {
            continue; // 跳过读取失败的块
        }

        // 遍历块中的目录项，从当前偏移开始
        Ext2DirEntry* entry = (Ext2DirEntry*)(block + (i == block_idx ? block_offset : 0));
        if (entry->rec_len == 0) {
            break; // 空目录项，结束遍历
        }
        while((uint8_t*)entry < block + block_size) {
            log_debug("read entry at 0x%x\n", entry);
            entry->print();
            if(entry->inode) { // 有效的目录项
                // 获取文件类型
                Ext2Inode* entry_inode = m_fs->read_inode(entry->inode);
                uint8_t d_type = 0; // DT_UNKNOWN
                if(entry_inode) {
                    if((entry_inode->mode & 0xF000) == 0x4000) {
                        d_type = 2; // DT_DIR
                    } else if((entry_inode->mode & 0xF000) == 0x8000) {
                        d_type = 1; // DT_REG
                    }
                    delete entry_inode;
                }

                // 计算dirent结构体大小
                uint16_t reclen = sizeof(dirent) + entry->name_len + 1; // +1 for null terminator
                // 对齐到4字节边界
                reclen = (reclen + 3) & ~3;

                // 检查缓冲区是否足够
                if(bytes_written + reclen > buffer_size) {
                    break; // 缓冲区已满
                }

                // 填充dirent结构体
                dirent* dent = (dirent*)(output_buffer + bytes_written);
                dent->d_ino = entry->inode;
                dent->d_off = current_pos + ((uint8_t*)entry - block);
                dent->d_reclen = reclen;
                dent->d_type = d_type;
                memcpy(dent->d_name, entry->name, entry->name_len);
                dent->d_name[entry->name_len] = '\0'; // 添加null终止符

                bytes_written += reclen;
            }

            // 更新当前位置
            current_pos += entry->rec_len;

            // 移动到下一个目录项
            entry = (Ext2DirEntry*)((uint8_t*)entry + entry->rec_len);

            // 如果已经填满缓冲区，退出循环
            if(bytes_written + sizeof(dirent) > buffer_size) {
                break;
            }
        }

        // 重置块内偏移，因为下一个块从头开始
        block_offset = 0;
    }

    // 更新位置指针
    log_debug("update pos: %d(0x%x)\n", current_pos, current_pos);
    *pos = current_pos;

    delete[] block;
    delete inode;

    return bytes_written; // 返回写入的字节数
}

/**
 * get block id (index number under block device)
 * @param block_idx block index under an inode
 * @param inode
 * @return block index under block device
 */
uint32_t Ext2FileDescriptor::get_block_id(uint32_t block_idx, Ext2Inode *inode)
{
    auto dev = m_fs->device;
    if (block_idx < 12) {
        return inode->i_block[block_idx];
    } else if (block_idx < 12+ 256) {
        uint32_t indirect_block_id = inode->i_block[12];
        uint32_t indirect_offset = (block_idx - 12) * sizeof(uint32_t);

        uint32_t block_id;

        auto key = PageKey{indirect_block_id};
        auto page = m_fs->page_cache->get_page(key);

        block_id = *((uint32_t*)(page->data + indirect_offset));
        log_debug("inode_block_idx:%d, indirect_block_id:%d, block_id:%d\n", block_idx, indirect_block_id, block_id);
        return block_id;
    } else if (block_idx < 12 + 256 + 256*256) {
        uint32_t indirect_block = inode->i_block[13];
        auto page = m_fs->page_cache->get_page(PageKey{indirect_block});
        uint32_t indirect_block_offset = ((block_idx - 12 - 256)%(256*256));
        uint32_t double_indirect_block = *(uint32_t*)(page->data + indirect_block_offset*sizeof(uint32_t));
        uint32_t double_indirect_offset = (block_idx - 12 - 256)%256;
        auto page2 = m_fs->page_cache->get_page(PageKey{double_indirect_block});
        uint32_t block_id = *((uint32_t*)(page2->data + double_indirect_offset*sizeof(uint32_t)));
        return block_id;
    }
    return 0;
}

ssize_t Ext2FileDescriptor::read(void* buffer, size_t size)
{
    Ext2Inode* inode = m_fs->read_inode(m_inode);
    if(!inode)
        return -1;

    inode->print();

    uint32_t block_size = m_fs->device->block_size();
    size_t bytes_remaining = inode->size - m_position;
    size_t bytes_to_read = min(size, bytes_remaining);
    size_t total_read = 0;



    while(bytes_to_read > 0) {
        // 计算当前块号
        uint32_t inode_block_idx = m_position / block_size;
        uint32_t data_offset = m_position % block_size;

        auto device_block_id = get_block_id(m_position/block_size, inode);
        auto page = m_fs->page_cache->get_page(PageKey{device_block_id});

        // 复制数据到缓冲区
        size_t copy_size = min(block_size - data_offset, bytes_to_read);
        memcpy(static_cast<uint8_t*>(buffer) + total_read, page->data + data_offset, copy_size);

        m_position += copy_size;
        total_read += copy_size;
        bytes_to_read -= copy_size;
    }

    delete inode;
    return total_read;
}

// 在Ext2FileSystem类实现之外添加
ssize_t Ext2FileDescriptor::write(const void* buffer, size_t size)
{
    Ext2Inode* inode = m_fs->read_inode(m_inode);
    if(!inode || (inode->mode & 0xF000) == 0x4000) { // 目录不可写
        delete inode;
        return 0;
    }

    uint32_t block_size = m_fs->device->get_info().sector_size;
    const uint8_t* src = static_cast<const uint8_t*>(buffer);
    size_t total_written = 0;

    while(total_written < size) {
        // 计算当前块索引和偏移
        uint32_t block_idx = m_position / block_size;
        uint32_t block_offset = m_position % block_size;

        // 需要分配新块的情况
        if(block_idx >= inode->blocks) {
            uint32_t new_block = m_fs->allocate_block();
            if(!new_block)
                break;

            inode->i_block[block_idx] = new_block;
            inode->blocks++;
        }

        // 读取现有块数据（如果需要部分写入）
        uint8_t* block_data = new uint8_t[block_size];
        if(block_offset > 0 || (size - total_written) < block_size) {
            m_fs->device->read_block(inode->i_block[block_idx], block_data);
        }

        // 计算本次写入量
        size_t write_size = min(size - total_written, block_size - block_offset);
        memcpy(block_data + block_offset, src + total_written, write_size);

        // 写入块设备
        if(!m_fs->device->write_block(inode->i_block[block_idx], block_data)) {
            delete[] block_data;
            break;
        }

        // 更新状态
        total_written += write_size;
        m_position += write_size;
        delete[] block_data;

        // 扩展文件大小
        if(m_position > inode->size) {
            inode->size = m_position;
        }
    }

    // 更新inode信息
    // inode->mtime = time(nullptr);
    m_fs->write_inode(m_inode, inode);
    delete inode;

    return total_written;
}
void Ext2DirEntry::print()
{
    log_debug(
        "dir:%s, inode: %u, rec_len: %u, name_len: %u\n", name, inode, rec_len, name_len, name_len);
}

} // namespace kernel