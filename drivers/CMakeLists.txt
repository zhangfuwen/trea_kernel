# 创建驱动核心库
add_library(drivers_core
    keyboard.cpp
    block_device.cpp
    ext2.cpp
        ../include/drivers/keyboard.h
)

# 添加包含目录
target_include_directories(drivers_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(drivers_core PRIVATE kernel_lib)

# 编译选项
target_compile_options(drivers_core PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
        ${OS_COMPILE_OPTIONS}
)
target_link_options(drivers_core PRIVATE ${OS_LINK_OPTIONS})