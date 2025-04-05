# 添加子目录
add_subdirectory(memory)
add_subdirectory(process)
add_subdirectory(fs)
add_subdirectory(smp)
add_subdirectory(core)

# 创建内核核心库
add_library(kernel_core
    ../lib/string.cpp
    ../arch/x86/gdt.cpp
)

# 链接所有子模块
target_link_libraries(kernel_core
    kernel_memory
    kernel_process
    kernel_fs
    kernel_smp
    kernel_core_main
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
