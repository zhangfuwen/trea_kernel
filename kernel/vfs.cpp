#include <kernel/vfs.h>
#include <lib/string.h>
#include <lib/console.h>

namespace kernel {

// 最大挂载点数量
const int MAX_MOUNT_POINTS = 16;

// 挂载点结构
struct MountPoint {
    char path[256];
    FileSystem* fs;
    bool used;
};

// 挂载点列表
static MountPoint mount_points[MAX_MOUNT_POINTS];

// 初始化VFS
void init_vfs() {
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        mount_points[i].used = false;
    }
}

// 查找挂载点
static FileSystem* find_fs(const char* path, const char** remaining_path) {
    size_t longest_match = 0;
    FileSystem* matched_fs = nullptr;
    
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (!mount_points[i].used) continue;
        
        size_t mount_len = strlen(mount_points[i].path);
        if (strncmp(path, mount_points[i].path, mount_len) == 0) {
            if (mount_len > longest_match) {
                longest_match = mount_len;
                matched_fs = mount_points[i].fs;
                *remaining_path = path + mount_len;
            }
        }
    }
    
    return matched_fs;
}

void VFSManager::register_fs(const char* mount_point, FileSystem* fs) {
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (!mount_points[i].used) {
            strncpy(mount_points[i].path, mount_point, sizeof(mount_points[i].path) - 1);
            mount_points[i].fs = fs;
            mount_points[i].used = true;
            return;
        }
    }
    Console::print("No free mount points available\n");
}

FileDescriptor* VFSManager::open(const char* path) {
    const char* remaining_path;
    FileSystem* fs = find_fs(path, &remaining_path);
    if (!fs) return nullptr;
    return fs->open(remaining_path);
}

int VFSManager::stat(const char* path, FileAttribute* attr) {
    const char* remaining_path;
    FileSystem* fs = find_fs(path, &remaining_path);
    if (!fs) return -1;
    return fs->stat(remaining_path, attr);
}

int VFSManager::mkdir(const char* path) {
    const char* remaining_path;
    FileSystem* fs = find_fs(path, &remaining_path);
    if (!fs) return -1;
    return fs->mkdir(remaining_path);
}

int VFSManager::unlink(const char* path) {
    const char* remaining_path;
    FileSystem* fs = find_fs(path, &remaining_path);
    if (!fs) return -1;
    return fs->unlink(remaining_path);
}

int VFSManager::rmdir(const char* path) {
    const char* remaining_path;
    FileSystem* fs = find_fs(path, &remaining_path);
    if (!fs) return -1;
    return fs->rmdir(remaining_path);
}

} // namespace kernel