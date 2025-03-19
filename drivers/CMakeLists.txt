# 创建驱动核心库
add_library(drivers_core
    keyboard.cpp
)

# 添加包含目录
target_include_directories(drivers_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 编译选项
target_compile_options(drivers_core PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
)