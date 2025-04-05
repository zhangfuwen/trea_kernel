#include "kernel/elf_loader.h"
#include "lib/debug.h"

// 处理重定位表
bool ElfLoader::process_relocations(const void* elf_data, const Rel* rel, uint32_t rel_size,
    const Symbol* symtab, const char* strtab, uint32_t base_address)
{
    for(uint32_t i = 0; i < rel_size; i++) {
        // 获取重定位类型和符号索引
        uint32_t type = rel[i].info & 0xFF;
        uint32_t sym_idx = rel[i].info >> 8;

        // 获取重定位地址
        uint32_t* rel_addr = reinterpret_cast<uint32_t*>(rel[i].offset + base_address);

        // 获取符号值
        uint32_t sym_value = get_symbol_value(&symtab[sym_idx], base_address);

        debug_debug("Relocation: type=%d, offset=%x, symbol=%d, value=%x\n", type, rel[i].offset,
            sym_idx, sym_value);

        // 根据重定位类型进行处理
        switch(type) {
            case R_386_NONE:
                // 无操作
                break;

            case R_386_32: // 绝对寻址
                *rel_addr = sym_value;
                break;

            case R_386_PC32: // PC相对寻址
                *rel_addr = sym_value - (rel[i].offset + base_address);
                break;

            case R_386_GLOB_DAT: // 设置GOT表项
            case R_386_JMP_SLOT: // 设置PLT表项
                *rel_addr = sym_value;
                break;

            case R_386_RELATIVE: // 基址相对寻址
                *rel_addr += base_address;
                break;

            default:
                debug_err("Unsupported relocation type: %d\n", type);
                return false;
        }
    }

    return true;
}

// 处理动态链接信息
bool ElfLoader::process_dynamic(
    const void* elf_data, const Dyn* dynamic, uint32_t dynamic_size, uint32_t base_address)
{
    const Rel* rel = nullptr;
    uint32_t rel_size = 0;
    const Rel* plt_rel = nullptr;
    uint32_t plt_rel_size = 0;
    const Symbol* dynsym = nullptr;
    const char* dynstr = nullptr;

    // 第一遍：查找动态链接相关表
    for(uint32_t i = 0; i < dynamic_size; i++) {
        switch(dynamic[i].tag) {
            case DT_SYMTAB:
                dynsym = reinterpret_cast<const Symbol*>(
                    static_cast<const char*>(elf_data) + dynamic[i].d_un.ptr);
                break;

            case DT_STRTAB:
                dynstr = static_cast<const char*>(elf_data) + dynamic[i].d_un.ptr;
                break;

            case DT_REL:
                rel = reinterpret_cast<const Rel*>(
                    static_cast<const char*>(elf_data) + dynamic[i].d_un.ptr);
                break;

            case DT_RELSZ:
                rel_size = dynamic[i].d_un.val / sizeof(Rel);
                break;

            case DT_JMPREL:
                plt_rel = reinterpret_cast<const Rel*>(
                    static_cast<const char*>(elf_data) + dynamic[i].d_un.ptr);
                break;

            case DT_PLTRELSZ:
                plt_rel_size = dynamic[i].d_un.val / sizeof(Rel);
                break;
        }
    }

    // 处理常规重定位表
    if(rel != nullptr && rel_size > 0 && dynsym != nullptr && dynstr != nullptr) {
        debug_debug("Processing dynamic relocations, count: %d\n", rel_size);
        if(!process_relocations(elf_data, rel, rel_size, dynsym, dynstr, base_address)) {
            return false;
        }
    }

    // 处理PLT重定位表
    if(plt_rel != nullptr && plt_rel_size > 0 && dynsym != nullptr && dynstr != nullptr) {
        debug_debug("Processing PLT relocations, count: %d\n", plt_rel_size);
        if(!process_relocations(elf_data, plt_rel, plt_rel_size, dynsym, dynstr, base_address)) {
            return false;
        }
    }

    return true;
}

// 获取符号值
uint32_t ElfLoader::get_symbol_value(const Symbol* sym, uint32_t base_address)
{
    // 如果符号值为0，表示未定义符号，返回0
    if(sym->value == 0) {
        return 0;
    }

    // 返回符号值加上基地址
    return sym->value + base_address;
}