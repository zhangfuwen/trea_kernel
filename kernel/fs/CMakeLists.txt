# 文件系统相关代码
add_library(kernel_fs
    vfs.cpp
    memfs.cpp
    console_device.cpp
        SimplePageCache.cpp
)

# 添加包含目录
target_include_directories(kernel_fs PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(kernel_fs PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
)
target_link_libraries(kernel_fs PUBLIC kernel_lib)
target_compile_options(kernel_fs PRIVATE ${OS_COMPILE_OPTIONS})
target_link_options(kernel_fs PRIVATE ${OS_LINK_OPTIONS})