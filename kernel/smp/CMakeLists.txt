# 多处理器支持相关代码
add_library(kernel_smp STATIC
        ../../lib/mutex.cpp
    smp_scheduler.cpp
)

# 添加包含目录
target_include_directories(kernel_smp PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(kernel_smp PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
)
target_compile_options(kernel_smp PRIVATE ${OS_COMPILE_OPTIONS})
target_link_options(kernel_smp PRIVATE ${OS_LINK_OPTIONS})