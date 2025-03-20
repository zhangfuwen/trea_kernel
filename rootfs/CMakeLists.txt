# 创建内核库
add_executable(init
    init.cpp
)

# 添加包含目录
target_include_directories(init PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(init PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
)