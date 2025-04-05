# 内存管理相关代码
add_library(kernel_memory
    buddy_allocator.cpp
    slab_allocator.cpp
    memory_operators.cpp
    kernel_memory.cpp
    user_memory.cpp
    virtual_memory_tree.cpp
    paging.cpp
    zone.cpp
)

# 添加包含目录
target_include_directories(kernel_memory PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(kernel_memory PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
)
target_compile_options(kernel_memory PRIVATE ${OS_COMPILE_OPTIONS})
target_link_options(kernel_memory PRIVATE ${OS_LINK_OPTIONS})