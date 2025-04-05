# 内核核心代码
add_library(kernel_core_main
    main.cpp
    kernel.cpp
    syscall.cpp
    ../../arch/x86/gdt.cpp
)

# 添加包含目录
target_include_directories(kernel_core_main PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(kernel_core_main PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
)
target_compile_options(kernel_core_main PRIVATE ${OS_COMPILE_OPTIONS})
target_link_options(kernel_core_main PRIVATE ${OS_LINK_OPTIONS})