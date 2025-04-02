# 创建内核库
add_library(kernel_lib
    string.cpp
    console.cpp
    serial.cpp
    debug.cpp
    debug_test.cpp
)

# 添加包含目录
target_include_directories(kernel_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(kernel_lib PRIVATE ${OS_COMPILE_OPTIONS})
target_link_options(kernel_lib PRIVATE ${OS_LINK_OPTIONS})
