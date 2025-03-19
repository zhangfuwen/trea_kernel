#include "../arch/x86/gdt.cpp"
#include "../arch/x86/idt.cpp"
#include "../arch/x86/interrupt.cpp"
#include "../arch/x86/paging.cpp"

extern "C" void kernel_main() {
    // 初始化GDT
    GDT::init();

    // 初始化IDT
    IDT::init();

    // 初始化中断管理器
    InterruptManager::init();

    // 初始化分页
    PageManager::init();

    // 进入无限循环，等待中断
    while (true) {
        asm volatile("hlt");
    }
}