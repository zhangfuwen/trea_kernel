# 创建内核核心库
add_library(kernel_core
    main.cpp
    process.cpp
    scheduler.cpp
    elf_loader.cpp
    elf_loader_reloc.cpp
    buddy_allocator.cpp
        memory_operators.cpp
        console_device.cpp
    memfs.cpp
    syscall.cpp
    vfs.cpp
    ../lib/string.cpp
    ../arch/x86/gdt.cpp
        exit_handler.cpp
        process_manager.cpp
        process_wrapper.cpp
        user_memory.cpp
        kernel.cpp
        zone.cpp
        kernel_memory.cpp
        virtual_memory_tree.cpp
        paging.cpp
)

# 添加包含目录
target_include_directories(kernel_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(kernel_core PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
)
target_compile_options(kernel_core PRIVATE ${OS_COMPILE_OPTIONS})
target_link_options(kernel_core PRIVATE ${OS_LINK_OPTIONS})
