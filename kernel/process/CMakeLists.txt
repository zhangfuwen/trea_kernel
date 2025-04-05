# 进程管理相关代码
add_library(kernel_process
    process.cpp
    process_manager.cpp
    process_wrapper.cpp
    scheduler.cpp
    elf_loader.cpp
    elf_loader_reloc.cpp
    exit_handler.cpp
)

# 添加包含目录
target_include_directories(kernel_process PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(kernel_process PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
)
target_compile_options(kernel_process PRIVATE ${OS_COMPILE_OPTIONS})
target_link_options(kernel_process PRIVATE ${OS_LINK_OPTIONS})