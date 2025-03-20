#pragma once
#include <stdint.h>
#include "lib/console.h"
#include "process.h"

// ELF文件头魔数
#define ELF_MAGIC 0x464C457F

// ELF文件类型
enum ElfType {
    ET_NONE = 0,        // 未知类型
    ET_REL = 1,         // 可重定位文件
    ET_EXEC = 2,        // 可执行文件
    ET_DYN = 3,         // 共享目标文件
    ET_CORE = 4         // Core文件
};

// ELF头部结构
struct ElfHeader {
    uint32_t magic;             // 魔数
    uint8_t ident[12];          // 标识信息
    uint16_t type;              // 文件类型
    uint16_t machine;           // 目标架构
    uint32_t version;           // 版本号
    uint32_t entry;             // 入口点地址
    uint32_t phoff;             // 程序头表偏移
    uint32_t shoff;             // 节头表偏移
    uint32_t flags;             // 标志
    uint16_t ehsize;            // ELF头大小
    uint16_t phentsize;         // 程序头表项大小
    uint16_t phnum;             // 程序头表项数量
    uint16_t shentsize;         // 节头表项大小
    uint16_t shnum;             // 节头表项数量
    uint16_t shstrndx;          // 字符串表索引
};

// 程序头表项结构
struct ProgramHeader {
    uint32_t type;              // 段类型
    uint32_t offset;            // 段偏移
    uint32_t vaddr;             // 虚拟地址
    uint32_t paddr;             // 物理地址
    uint32_t filesz;            // 文件中的大小
    uint32_t memsz;             // 内存中的大小
    uint32_t flags;             // 段标志
    uint32_t align;             // 对齐
};

class ElfLoader {
public:
    static bool load_elf(const void* elf_data, uint32_t size);
    static void execute(uint32_t entry_point, ProcessControlBlock* process);
    static int exec(const char* path, char* const argv[]);
};