#pragma once
#include "block_device.h"
#include <kernel/vfs.h>
#include <stddef.h>
#include <cstdint>

namespace kernel
{

// EXT2文件系统常量
constexpr uint16_t EXT2_MAGIC = 0xEF53;
constexpr uint32_t EXT2_DIRECT_BLOCKS = 12;
constexpr uint32_t EXT2_NAME_LEN = 255;

// 超级块结构
struct Ext2SuperBlock {
    uint32_t inodes_count;      /* Inodes count */
    uint32_t blocks_count;      /* Blocks count */
    uint32_t r_blocks_count;    /* Reserved blocks count */
    uint32_t free_blocks_count; /* Free blocks count */
    uint32_t free_inodes_count; /* Free inodes count */
    uint32_t first_data_block;  /* First Data Block */
    uint32_t log_block_size;    /* Block size */
    uint32_t log_frag_size;     /* Fragment size */
    uint32_t blocks_per_group;  /* # Blocks per group */
    uint32_t frags_per_group;   /* # Fragments per group */
    uint32_t inodes_per_group;  /* # Inodes per group */
    uint32_t mtime;             /* Mount time */
    uint32_t wtime;             /* Write time */
    uint16_t mnt_count;         /* Mount count */
    uint16_t max_mnt_count;     /* Maximal mount count */
    uint16_t magic;             /* Magic signature */
    uint16_t state;             /* File system state */
    uint16_t errors;            /* Behaviour when detecting errors */
    uint16_t minor_rev_level;   /* minor revision level */
    uint32_t lastcheck;         /* time of last check */
    uint32_t checkinterval;     /* max. time between checks */
    uint32_t creator_os;        /* OS */
    uint32_t rev_level;         /* Revision level */
    uint16_t def_resuid;        /* Default uid for reserved blocks */
    uint16_t def_resgid;        /* Default gid for reserved blocks */
    /*
     * These fields are for EXT2_DYNAMIC_REV superblocks only.
     *
     * Note: the difference between the compatible feature set and
     * the incompatible feature set is that if there is a bit set
     * in the incompatible feature set that the kernel doesn't
     * know about, it should refuse to mount the filesystem.
     *
     * e2fsck's requirements are more strict; if it doesn't know
     * about a feature in either the compatible or incompatible
     * feature set, it must abort and not try to meddle with
     * things it doesn't understand...
     */
    uint32_t first_ino;              /* First non-reserved inode */
    uint16_t inode_size;             /* size of inode structure */
    uint16_t block_group_nr;         /* block group # of this superblock */
    uint32_t feature_compat;         /* compatible feature set */
    uint32_t feature_incompat;       /* incompatible feature set */
    uint32_t feature_ro_compat;      /* readonly-compatible feature set */
    uint8_t uuid[16];                /* 128-bit uuid for volume */
    char s_volume_name[16];          /* volume name */
    char s_last_mounted[64];         /* directory where last mounted */
    uint32_t algorithm_usage_bitmap; /* For compression */
    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT2_COMPAT_PREALLOC flag is on.
     */
    uint8_t prealloc_blocks;     /* Nr of blocks to try to preallocate*/
    uint8_t prealloc_dir_blocks; /* Nr to preallocate for dirs */
    uint16_t padding1;
    /*
     * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
     */
    uint8_t journal_uuid[16]; /* uuid of journal superblock */
    uint32_t journal_inum;    /* inode number of journal file */
    uint32_t journal_dev;     /* device number of journal file */
    uint32_t last_orphan;     /* start of list of inodes to delete */
    uint32_t hash_seed[4];    /* HTREE hash seed */
    uint8_t def_hash_version; /* Default hash version to use */
    uint8_t reserved_char_pad;
    uint16_t reserved_word_pad;
    uint32_t default_mount_opts;
    uint32_t first_meta_bg; /* First metablock block group */
    uint32_t reserved[190]; /* Padding to the end of the block */
    uint32_t block_size() { return 1024 << log_block_size; }
    void print();
};

#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS (EXT2_TIND_BLOCK + 1)

/*
 * Special inode numbers
 */
#define EXT2_BAD_INO 1         /* Bad blocks inode */
#define EXT2_ROOT_INO 2        /* Root inode */
#define EXT2_BOOT_LOADER_INO 5 /* Boot loader inode */
#define EXT2_UNDEL_DIR_INO 6   /* Undelete directory inode */

/* First non-reserved inode for old ext2 filesystems */
#define EXT2_GOOD_OLD_FIRST_INO 11
// Inode结构
struct Ext2Inode {
    uint16_t mode; // 文件类型和权限
    uint16_t uid;
    uint32_t size; // 文件大小
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t i_links_count; // 链接计数
    uint32_t blocks;        // 占用的块数
    uint32_t flags;
    uint32_t osd1;
    uint32_t i_block[EXT2_N_BLOCKS]; // 数据块指针
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    struct {
        uint8_t l_i_frag;  /* Fragment number */
        uint8_t l_i_fsize; /* Fragment size */
        uint16_t i_pad1;
        uint16_t l_i_uid_high; /* these 2 fields    */
        uint16_t l_i_gid_high; /* were reserved2[0] */
        uint32_t l_i_reserved2;
    } osd2;
    static uint32_t inode_table_block();
    void print();
};
/*
 * Structure of a blocks group descriptor
 */
struct Ext2GroupDesc {
    uint32_t bg_block_bitmap;      /* Blocks bitmap block */
    uint32_t bg_inode_bitmap;      /* Inodes bitmap block */
    uint32_t bg_inode_table;       /* Inodes table block */
    uint16_t bg_free_blocks_count; /* Free blocks count */
    uint16_t bg_free_inodes_count; /* Free inodes count */
    uint16_t bg_used_dirs_count;   /* Directories count */
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
    void print();
};

// 目录项结构
struct Ext2DirEntry {
    uint32_t inode;           // Inode号
    uint16_t rec_len;         // 目录项长度
    uint8_t name_len;         // 名称长度
    uint8_t file_type;        // 文件类型
    char name[EXT2_NAME_LEN]; // 文件名
    void print();
};

class Ext2FileDescriptor;
// EXT2文件系统实现
class Ext2FileSystem : public FileSystem
{
public:
    Ext2FileSystem(BlockDevice* device);
    virtual ~Ext2FileSystem();

    // 实现VFS接口
    virtual char* get_name() override;
    virtual FileDescriptor* open([[maybe_unused]] const char* path) override;
    virtual int stat([[maybe_unused]] const char* path, FileAttribute* attr) override;
    virtual int mkdir([[maybe_unused]] const char* path) override;
    virtual int unlink([[maybe_unused]] const char* path) override;
    virtual int rmdir([[maybe_unused]] const char* path) override;

private:
    friend class Ext2FileDescriptor;
    BlockDevice* device;
    Ext2SuperBlock* super_block;
    Ext2GroupDesc group_desc;
    Ext2Inode root_inodes;
    uint32_t current_dir_inode = 2; // 默认根目录

    // 内部辅助函数
    bool read_super_block();
    Ext2Inode* read_inode(uint32_t inode_num);
    bool write_inode(uint32_t inode_num, const Ext2Inode* inode);
    uint32_t allocate_block();
    uint32_t allocate_inode();
};

class Ext2FileDescriptor : public kernel::FileDescriptor
{
public:
    Ext2FileDescriptor(uint32_t inode, Ext2FileSystem* fs);
    ~Ext2FileDescriptor() override;

    ssize_t read(void* buffer, size_t size) override;
    ssize_t write(const void* buffer, size_t size) override;
    int seek([[maybe_unused]] size_t offset) override;
    int close() override;
    int iterate([[maybe_unused]] void* buffer, [[maybe_unused]] size_t buffer_size, [[maybe_unused]] uint32_t* pos) override;

private:
    friend class Ext2FileSystem;
    uint32_t m_inode;
    off_t m_position;
    Ext2FileSystem* m_fs;
};

} // namespace kernel
