# 创建内核库
add_executable(init
    init.cpp
    commands.cpp
    cmds/ls.cpp
    cmds/cat.cpp
    cmds/rm.cpp
    cmds/cd.cpp
    cmds/echo.cpp
    cmds/find.cpp
    cmds/xxd.cpp
    linker.ld
    init.asm
    utils.h
    utils.cpp
)
set_source_files_properties(
        init.asm
        PROPERTIES
        LANGUAGE ASM_NASM
        COMPILE_FLAGS "-f elf32 -g -F dwarf"
)

# 添加包含目录
target_include_directories(init PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(init kernel_lib)

set_target_properties(init PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/rootfs_bin)


message(STATUS "linker.ld: ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")
target_link_options(init PRIVATE
        "-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld"
)
# 编译选项
target_compile_options(init PRIVATE
    -ffreestanding
    -O2
    -Wall
    -Wextra
    -fPIC
)
target_compile_options(init PRIVATE
        ${OS_COMPILE_OPTIONS}
        -I${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_options(init PRIVATE ${OS_LINK_OPTIONS})

