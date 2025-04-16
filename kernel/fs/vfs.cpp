#include <arch/x86/interrupt.h>
#include <kernel/syscall.h>
#include <kernel/vfs.h>
#include <lib/console.h>
#include <lib/debug.h>
#include <lib/string.h>

namespace kernel
{

// 最大挂载点数量
const int MAX_MOUNT_POINTS = 16;

// 挂载点结构
struct MountPoint {
    FileSystem* fs;
    char path[256];
    bool used;
};

// 挂载点列表
static MountPoint mount_points[MAX_MOUNT_POINTS];

// 打开文件系统调用处理函数
int openHandler(uint32_t path_ptr, uint32_t b, uint32_t c, uint32_t d)
{
    auto pcb = ProcessManager::get_current_task();
    return sys_open(path_ptr, pcb);
}
int sys_open(uint32_t path_ptr, Task* task)
{
    debug_debug("openHandler called with path: 0x%x, pcb:0x%x(pid:%d)\n", path_ptr, task, task->task_id);

    const char* path = reinterpret_cast<const char*>(path_ptr);
    debug_debug("Opening file: %s\n", path);

    FileDescriptor* fd = VFSManager::instance().open(path);
    if(!fd) {
        debug_debug("Failed to open file: %s\n", path);
        return -1;
    }

    // 分配文件描述符
    int fd_num = task->context->allocate_fd();
    task->context->fd_table[fd_num] = fd;

    debug_debug("File opened successfully, pcb:0x%x, pid:%d, fd: %d\n", task, task->task_id, fd_num);
    return fd_num;
}

// 读取文件系统调用处理函数
int readHandler(uint32_t fd_num, uint32_t buffer_ptr, uint32_t size, uint32_t d)
{
    auto pcb = ProcessManager::get_current_task();
    return sys_read(fd_num, buffer_ptr, size, pcb);
}
int sys_read(uint32_t fd_num, uint32_t buffer_ptr, uint32_t size, Task* pcb)
{
    debug_debug("readHandler called with fd: %d, buffer: %x, size: %d\n", fd_num, buffer_ptr, size);

    if(fd_num >= 256 || !pcb->context->fd_table[fd_num]) {
        debug_debug("Invalid file descriptor: %d\n", fd_num);
        return -1;
    }

    void* buffer = reinterpret_cast<void*>(buffer_ptr);

    auto process = pcb;
    if(!process) {
        debug_debug("No current process\n");
        return -1;
    }
 //   debug_debug("process: %x\n", process);

    auto fd_table = process->context->fd_table;
    if(!fd_table[fd_num]) {
        debug_debug("File descriptor %d not open\n", fd_num);
        return -1;
    }
//    debug_debug("fd_table: %x\n", fd_table);

    auto fd = fd_table[fd_num];
    if(!fd) {
        debug_err("File descriptor %d not open\n", fd_num);
        return -1;
    }
    debug_debug("fd: %x\n", fd);

    ssize_t bytes_read = 0;
    bytes_read = fd->read(buffer, size);

    debug_debug("Read %d bytes from fd %d, buffer:0x%x, exit\n", bytes_read, fd_num, buffer);
    return bytes_read;
}

// 写入文件系统调用处理函数
int writeHandler(uint32_t fd_num, uint32_t buffer_ptr, uint32_t size, uint32_t d)
{
    auto pcb = ProcessManager::get_current_task();
    return sys_write(fd_num, buffer_ptr, size, pcb);
}
int sys_write(uint32_t fd_num, uint32_t buffer_ptr, uint32_t size, Task* task)
{
    debug_debug(
        "writeHandler called with fd: %d, buffer: %d, size: %d\n", fd_num, buffer_ptr, size);

    auto context = task->context;
    if(fd_num >= 256 || !context->fd_table[fd_num]) {
        debug_debug("Invalid file descriptor, pcb:0x%x, pid:%d, fd: %d\n", task, task->task_id, fd_num);
        return -1;
    }

    const void* buffer = reinterpret_cast<const void*>(buffer_ptr);
    ssize_t bytes_written = context->fd_table[fd_num]->write(buffer, size);

    debug_debug("Wrote %d bytes to fd %d\n", bytes_written, fd_num);
    return bytes_written;
}

// 关闭文件系统调用处理函数
int closeHandler(uint32_t fd_num, uint32_t b, uint32_t c, uint32_t d)
{
    auto pcb = ProcessManager::get_current_task();
    return sys_close(fd_num, pcb);
}
int sys_close(uint32_t fd_num, Task* pcb)
{
    debug_debug("closeHandler called with fd: %d\n", fd_num);

    if(fd_num >= 256 || !pcb->context->fd_table[fd_num]) {
        debug_debug("Invalid file descriptor: %d\n", fd_num);
        return -1;
    }

    int result = pcb->context->fd_table[fd_num]->close();
    pcb->context->fd_table[fd_num] = nullptr;

    debug_debug("File descriptor %d closed\n", fd_num);
    return result;
}

// 文件指针定位系统调用处理函数
int seekHandler(uint32_t fd_num, uint32_t offset, uint32_t c, uint32_t d)
{
    auto pcb = ProcessManager::get_current_task();
    auto ret = sys_seek(fd_num, offset, pcb);
    return ret;
}
int sys_seek(uint32_t fd_num, uint32_t offset, Task* pcb)
{
    debug_debug("seekHandler called with fd: %d, offset: %d\n", fd_num, offset);

    if(fd_num >= 256 || !pcb->context->fd_table[fd_num]) {
        debug_debug("Invalid file descriptor: %d\n", fd_num);
        return -1;
    }

    int result = pcb->context->fd_table[fd_num]->seek(offset);

    debug_debug("Seek result: %d\n", result);
    return result;
}
// 初始化VFS
void init_vfs()
{
    for(int i = 0; i < MAX_MOUNT_POINTS; i++) {
        mount_points[i].used = false;
    }
    // 注册文件系统相关的系统调用处理函数
    SyscallManager::registerHandler(SYS_OPEN, openHandler);
    SyscallManager::registerHandler(SYS_READ, readHandler);
    SyscallManager::registerHandler(SYS_WRITE, writeHandler);
    SyscallManager::registerHandler(SYS_CLOSE, closeHandler);
    SyscallManager::registerHandler(SYS_SEEK, seekHandler);
    SyscallManager::registerHandler(SYS_GETDENTS, getdentsHandler);

    debug_debug("VFS initialized and system calls registered\n");
}

// 查找挂载点
static FileSystem* find_fs(const char* path, const char** remaining_path)
{
    size_t longest_match = 0;
    FileSystem* matched_fs = nullptr;

    for(int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if(!mount_points[i].used)
            continue;

        debug_debug(
            "VFSManager::find_fs: checking path %s against %s\n", path, mount_points[i].path);

        size_t mount_len = strlen(mount_points[i].path);
        debug_debug("VFSManager::find_fs: mount_len %d\n", mount_len);

        int ret = strncmp(path, mount_points[i].path, mount_len);
        if(ret == 0) {
            //  debug_debug("VFSManager::find_fs: found %x\n", mount_points[i].fs);
            // debug_debug("VFSManager::find_fs: found %s\n",
            // mount_points[i].fs->get_name()); debug_debug("VFSManager::find_fs:
            // found filesystem %s for path %s\n", mount_points[i].fs->get_name(),
            // path); 找到最长匹配的挂载点，避免匹配到 /usr/bin 时也匹配到 /usr/bin/ls
            // 这种情况
            if(mount_len > longest_match) {
                longest_match = mount_len;
                //     debug_debug("VFSManager::find_fs: matched_fs %x\n", matched_fs);
                //    debug_debug("VFSManager::find_fs: matched_fs %x\n", matched_fs);
                matched_fs = mount_points[i].fs;
                *remaining_path = path + mount_len;
            }
        } else {
            debug_debug("VFSManager::find_fs: ret %d, path %s does not match %s\n", ret, path,
                mount_points[i].path);
        }
    }

    debug_debug("VFSManager::find_fs: matched_fs %x\n", matched_fs);
    return matched_fs;
}

void VFSManager::register_fs(const char* mount_point, FileSystem* fs)
{
    for(int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if(!mount_points[i].used) {
            strncpy(mount_points[i].path, mount_point, sizeof(mount_points[i].path) - 1);
            mount_points[i].fs = fs;
            mount_points[i].used = true;
            return;
        }
    }
    Console::print("No free mount points available\n");
}

FileDescriptor* VFSManager::open(const char* path)
{
    debug_debug("VFSManager::open called with %s\n", path);
    const char* remaining_path;
    FileSystem* fs = find_fs(path, &remaining_path);
    if(!fs) {
        debug_debug("VFSManager::open: no filesystem found for path %s\n", path);
        return nullptr;
    } else {
        debug_debug("VFSManager::open: filesystem found for path %s, fs at 0x%x\n", path, fs);
        uint32_t x = *(uint32_t*)fs;
        debug_debug("VFSManager::open: %x\n", x);
        fs->print();
        debug_debug("VFSManager::open: found filesystem %s for path %s\n", fs->get_name(), path);
    }

    return fs->open(remaining_path);
}

int VFSManager::stat(const char* path, FileAttribute* attr)
{
    const char* remaining_path;
    FileSystem* fs = find_fs(path, &remaining_path);
    if(!fs)
        return -1;
    return fs->stat(remaining_path, attr);
}

int VFSManager::mkdir(const char* path)
{
    const char* remaining_path;
    FileSystem* fs = find_fs(path, &remaining_path);
    if(!fs)
        return -1;
    return fs->mkdir(remaining_path);
}

int VFSManager::unlink(const char* path)
{
    const char* remaining_path;
    FileSystem* fs = find_fs(path, &remaining_path);
    if(!fs)
        return -1;
    return fs->unlink(remaining_path);
}

int VFSManager::rmdir(const char* path)
{
    const char* remaining_path;
    FileSystem* fs = find_fs(path, &remaining_path);
    if(!fs)
        return -1;
    return fs->rmdir(remaining_path);
}

int VFSManager::chdir(const char* path)
{
    debug_debug("VFSManager::chdir: changing to %s\n", path);

    // 获取对应的文件系统
    const char* remaining_path;
    FileSystem* fs = find_fs(path, &remaining_path);
    if (!fs) {
        debug_debug("VFSManager::chdir: no filesystem found for %s\n", path);
        return -1;
    }

    // 检查目标路径是否存在且为目录
    FileAttribute attr;
    if (fs->stat(remaining_path, &attr) < 0) {
        debug_debug("VFSManager::chdir: failed to stat %s\n", remaining_path);
        return -1;
    }

    if (attr.type != FT_DIR) {
        debug_debug("VFSManager::chdir: %s is not a directory\n", remaining_path);
        return -1;
    }

    // 更新当前进程的工作目录
    auto pcb = ProcessManager::get_current_task();
    auto context = pcb->context;
    strncpy(pcb->context->cwd, path, sizeof(context->cwd) - 1);
    context->cwd[sizeof(context->cwd) - 1] = '\0';

    debug_debug("VFSManager::chdir: changed to %s\n", path);
    strcpy(current_working_directory, path);
    return 0;
}

} // namespace kernel