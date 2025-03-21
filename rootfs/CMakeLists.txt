# 创建内核库
add_executable(init
    init.cpp
)

# 添加包含目录
target_include_directories(init PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(init kernel_lib)

set_target_properties(init PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/rootfs_bin)



# 编译选项
target_compile_options(init PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
    -fPIC
)

