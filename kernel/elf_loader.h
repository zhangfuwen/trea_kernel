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

// 节头表类型
enum SectionType {
    SHT_NULL = 0,           // 未使用
    SHT_PROGBITS = 1,       // 程序数据
    SHT_SYMTAB = 2,         // 符号表
    SHT_STRTAB = 3,         // 字符串表
    SHT_RELA = 4,           // 带附加信息的重定位表
    SHT_HASH = 5,           // 符号哈希表
    SHT_DYNAMIC = 6,        // 动态链接信息
    SHT_NOTE = 7,           // 注释信息
    SHT_NOBITS = 8,         // 未初始化数据
    SHT_REL = 9,            // 不带附加信息的重定位表
    SHT_DYNSYM = 11         // 动态链接符号表
};

// 重定位类型
enum RelocationType {
    R_386_NONE = 0,          // 无操作
    R_386_32 = 1,            // 绝对寻址
    R_386_PC32 = 2,          // PC相对寻址
    R_386_GOT32 = 3,         // GOT表项偏移
    R_386_PLT32 = 4,         // PLT表项偏移
    R_386_COPY = 5,          // 复制符号
    R_386_GLOB_DAT = 6,      // 设置GOT表项
    R_386_JMP_SLOT = 7,      // 设置PLT表项
    R_386_RELATIVE = 8       // 基址相对寻址
};

// 程序头类型
#define PT_LOAD         1       /* Loadable segment */
#define PT_DYNAMIC      2       /* Dynamic linking information */
#define PT_INTERP       3       /* Interpreter path */
#define PT_NOTE         4       /* Note segment */
#define PT_SHLIB        5       /* Reserved */
#define PT_PHDR         6       /* Program header table */
#define PT_GNU_EH_FRAME 0x6474e550 /* GCC .eh_frame_hdr segment */

// 动态表项类型
#define DT_NULL         0       /* End of dynamic section */
#define DT_NEEDED       1       /* Name of needed library */
#define DT_PLTRELSZ     2       /* Size of PLT relocs */
#define DT_PLTGOT       3       /* Processor-specific */
#define DT_HASH         4       /* Dynamic symbol hash table */
#define DT_STRTAB       5       /* String table */
#define DT_SYMTAB       6       /* Symbol table */
#define DT_RELA         7       /* Relocation table */
#define DT_RELASZ       8       /* Size of relocation table */
#define DT_RELAENT      9       /* Size of relocation entry */
#define DT_STRSZ        10      /* Size of string table */
#define DT_SYMENT       11      /* Size of symbol table entry */
#define DT_INIT         12      /* Initialization function */
#define DT_FINI         13      /* Termination function */
#define DT_SONAME       14      /* Shared object name */
#define DT_RPATH        15      /* Library search path */
#define DT_REL          17      /* Relocation table */
#define DT_RELSZ        18      /* Size of relocation table */
#define DT_RELENT       19      /* Size of relocation entry */
#define DT_PLTREL       20      /* Type of PLT relocation */
#define DT_JMPREL       23      /* PLT relocation table */
#define DT_BIND_NOW     24      /* Process all relocations */
#define DT_INIT_ARRAY   25      /* Initialization function array */
#define DT_FINI_ARRAY   26      /* Termination function array */
#define DT_INIT_ARRAYSZ 27      /* Size of initialization array */
#define DT_FINI_ARRAYSZ 28      /* Size of termination array */

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

// 节头表项结构
struct SectionHeader {
    uint32_t name;              // 节名称（字符串表索引）
    uint32_t type;              // 节类型
    uint32_t flags;             // 节标志
    uint32_t addr;              // 虚拟地址
    uint32_t offset;            // 文件偏移
    uint32_t size;              // 节大小
    uint32_t link;              // 关联节索引
    uint32_t info;              // 附加信息
    uint32_t addralign;         // 地址对齐
    uint32_t entsize;           // 表项大小
};

// 符号表项结构
struct Symbol {
    uint32_t name;              // 符号名称（字符串表索引）
    uint32_t value;             // 符号值
    uint32_t size;              // 符号大小
    uint8_t info;               // 符号类型和绑定信息
    uint8_t other;              // 保留
    uint16_t shndx;             // 所在节索引
};

// 重定位表项结构（不带附加信息）
struct Rel {
    uint32_t offset;            // 重定位偏移
    uint32_t info;              // 符号索引和重定位类型
};

// 重定位表项结构（带附加信息）
struct Rela {
    uint32_t offset;            // 重定位偏移
    uint32_t info;              // 符号索引和重定位类型
    int32_t addend;             // 附加值
};

// 动态表项结构
struct Dyn {
    int32_t tag;                // 类型标记
    union {
        uint32_t val;           // 整数值
        uint32_t ptr;           // 指针值
    } d_un;
};


class ElfLoader {
public:
    // 加载ELF文件到指定的基地址
    // @param elf_data ELF文件数据
    // @param size 数据大小
    // @param base_address 加载的基地址，默认为0x100000
    // @return 是否加载成功
    static bool load_elf(const void* elf_data, uint32_t size, uint32_t base_address = 0x100000);
    
    // 执行加载的程序
    // @param entry_point 入口点地址
    // @param process 进程控制块
    static void execute(uint32_t entry_point, ProcessControlBlock* process);
    
    // 执行可执行文件
    // @param path 文件路径
    // @param argv 命令行参数
    // @return 执行结果
    static int exec(const char* path, char* const argv[]);

    static void switch_to_user_mode(uint32_t entry_point, uint32_t user_stack);

private:
    // 处理重定位表
    // @param elf_data ELF文件数据
    // @param rel 重定位表
    // @param rel_size 重定位表大小
    // @param symtab 符号表
    // @param strtab 字符串表
    // @param base_address 基地址
    // @return 是否处理成功
    static bool process_relocations(const void* elf_data, const Rel* rel, uint32_t rel_size, 
                                   const Symbol* symtab, const char* strtab, uint32_t base_address);
    
    // 处理动态链接信息
    // @param elf_data ELF文件数据
    // @param dynamic 动态表
    // @param dynamic_size 动态表大小
    // @param base_address 基地址
    // @return 是否处理成功
    static bool process_dynamic(const void* elf_data, const Dyn* dynamic, uint32_t dynamic_size, uint32_t base_address);
    
    // 获取符号值
    // @param sym 符号
    // @param base_address 基地址
    // @return 符号值
    static uint32_t get_symbol_value(const Symbol* sym, uint32_t base_address);
};