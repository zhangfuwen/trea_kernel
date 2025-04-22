#include "kernel/elf_loader.h"
#include "kernel/process.h"
#include "kernel/scheduler.h"
#include "lib/debug.h"
#include <arch/x86/gdt.h>
#include <kernel/vfs.h>

using namespace kernel;
extern "C" void set_user_entry(uint32_t entry, uint32_t stack);

// 加载ELF文件到指定的基地址
bool ElfLoader::load_elf(const void* elf_data, uint32_t size, uint32_t base_address)
{
    log_debug("Loading elf at %x, size:%d\n", elf_data, size);
    if(!elf_data || size < sizeof(ElfHeader)) {
        log_err("Invalid ELF file\n");
        return false;
    }

    const ElfHeader* header = static_cast<const ElfHeader*>(elf_data);

    // 检查ELF魔数
    if(header->magic != ELF_MAGIC) {
        log_err("Invalid ELF magic number, %x vs %x\n", header->magic, ELF_MAGIC);
        return false;
    }

    // 检查文件类型
    if(header->type != ET_EXEC && header->type != ET_DYN) {
        log_err("Not an executable or shared object file\n");
        return false;
    }

    // 加载程序段
    const ProgramHeader* ph = static_cast<const ProgramHeader*>(
        static_cast<const void*>(static_cast<const char*>(elf_data) + header->phoff));

    log_debug("num segments: %d\n", header->phnum);

    // 记录动态段的位置和大小，用于后续处理
    const Dyn* dynamic = nullptr;
    uint32_t dynamic_size = 0;

    // 第一遍：加载所有段
    for(uint16_t i = 0; i < header->phnum; i++) {
        if(ph[i].type == PT_LOAD) { // LOAD类型段
            // 分配内存并复制段内容
            void* dest = reinterpret_cast<void*>(ph[i].vaddr + base_address);
            const void* src =
                static_cast<const void*>(static_cast<const char*>(elf_data) + ph[i].offset);
            log_debug("Copying segment %x to %x (base: %x), size:%d\n", src, dest, base_address,
                ph[i].filesz);
            // 复制段内容到目标地址
            for(uint32_t j = 0; j < ph[i].filesz; j++) {
                static_cast<char*>(dest)[j] = static_cast<const char*>(src)[j];
            }
            // 清零剩余内存
            for(uint32_t j = ph[i].filesz; j < ph[i].memsz; j++) {
                static_cast<char*>(dest)[j] = 0;
            }
        } else if(ph[i].type == PT_DYNAMIC) {
            // 记录动态段的位置和大小，用于后续处理
            dynamic =
                reinterpret_cast<const Dyn*>(static_cast<const char*>(elf_data) + ph[i].offset);
            dynamic_size = ph[i].filesz / sizeof(Dyn);

            // 复制动态段到目标地址
            void* dest = reinterpret_cast<void*>(ph[i].vaddr + base_address);
            const void* src =
                static_cast<const void*>(static_cast<const char*>(elf_data) + ph[i].offset);
            log_debug("Copying dynamic segment %x to %x (base: %x)\n", src, dest, base_address);
            // 复制段内容到目标地址
            for(uint32_t j = 0; j < ph[i].filesz; j++) {
                static_cast<char*>(dest)[j] = static_cast<const char*>(src)[j];
            }
        } else if(ph[i].type == PT_GNU_EH_FRAME) {
            // 复制异常处理帧段
            void* dest = reinterpret_cast<void*>(ph[i].vaddr + base_address);
            const void* src =
                static_cast<const void*>(static_cast<const char*>(elf_data) + ph[i].offset);
            log_debug("Copying EH_FRAME segment %x to %x (base: %x)\n", src, dest, base_address);
            // 复制段内容到目标地址
            for(uint32_t j = 0; j < ph[i].filesz; j++) {
                static_cast<char*>(dest)[j] = static_cast<const char*>(src)[j];
            }
        }
    }

    // 如果是位置无关代码(ET_DYN)，需要处理重定位
    if(header->type == ET_DYN && dynamic != nullptr) {
        log_debug("Processing dynamic relocations...\n");
        if(!process_dynamic(elf_data, dynamic, dynamic_size, base_address)) {
            log_err("Failed to process dynamic relocations\n");
            return false;
        }
    }

    // 处理节头表中的重定位表
    if(header->shoff != 0 && header->shnum > 0) {
        const SectionHeader* sh = static_cast<const SectionHeader*>(
            static_cast<const void*>(static_cast<const char*>(elf_data) + header->shoff));

        // 查找字符串表
        const char* shstrtab = nullptr;
        if(header->shstrndx < header->shnum) {
            shstrtab = static_cast<const char*>(elf_data) + sh[header->shstrndx].offset;
        }

        // 查找符号表和重定位表
        const Symbol* symtab = nullptr;
        const char* strtab = nullptr;

        // 第一遍：查找符号表和字符串表
        for(uint16_t i = 0; i < header->shnum; i++) {
            if(sh[i].type == SHT_SYMTAB) {
                symtab = static_cast<const Symbol*>(
                    static_cast<const void*>(static_cast<const char*>(elf_data) + sh[i].offset));

                // 符号表关联的字符串表
                if(sh[i].link < header->shnum) {
                    strtab = static_cast<const char*>(elf_data) + sh[sh[i].link].offset;
                }
            }
        }

        // 第二遍：处理重定位表
        if(symtab != nullptr && strtab != nullptr) {
            for(uint16_t i = 0; i < header->shnum; i++) {
                if(sh[i].type == SHT_REL) {
                    const Rel* rel = static_cast<const Rel*>(static_cast<const void*>(
                        static_cast<const char*>(elf_data) + sh[i].offset));
                    uint32_t rel_size = sh[i].size / sizeof(Rel);

                    log_debug("Processing relocations from section %d...\n", i);
                    if(!process_relocations(
                           elf_data, rel, rel_size, symtab, strtab, base_address)) {
                        log_err("Failed to process relocations\n");
                        return false;
                    }
                }
            }
        }
    }

    log_debug("ELF loading completed successfully!\n");
    return true;
}