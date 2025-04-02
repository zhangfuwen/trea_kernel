cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(kernel_tests)

# 设置强制 VERBOSE 构建
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "")
set(CMAKE_CXX_LINKER_FLAGS "")
# 添加测试可执行文件
add_executable(format_string_test format_string_test.cpp)
add_executable(hexdump_test hexdump_test.cpp)

# 链接必要的库
target_link_libraries(format_string_test PRIVATE kernel_lib c gcc)
target_link_libraries(hexdump_test PRIVATE kernel_lib c gcc)

# 包含必要的头文件目录
target_include_directories(format_string_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
)

target_include_directories(hexdump_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
)

# 添加源文件
target_sources(format_string_test PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/debug.cpp
)

target_sources(hexdump_test PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/debug.cpp
)

# 移除从父工程传来的特定编译选项
get_target_property(COMPILE_OPTIONS format_string_test COMPILE_OPTIONS)
if(COMPILE_OPTIONS)
    list(REMOVE_ITEM COMPILE_OPTIONS "-ffreestanding" "-O2" "-nostdlib")
    set_target_properties(format_string_test PROPERTIES COMPILE_OPTIONS "${COMPILE_OPTIONS}")
endif()

get_target_property(COMPILE_OPTIONS hexdump_test COMPILE_OPTIONS)
if(COMPILE_OPTIONS)
    list(REMOVE_ITEM COMPILE_OPTIONS "-ffreestanding" "-O2" "-nostdlib")
    set_target_properties(hexdump_test PROPERTIES COMPILE_OPTIONS "${COMPILE_OPTIONS}")
endif()

message(STATUS "COMPILE_OPTIONS: ${COMPILE_OPTIONS}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

# 设置编译选项，启用标准库支持
target_compile_options(format_string_test PRIVATE
    -m32
    -Wall
    -Wextra
    -DTESTING
        -fno-builtin
)

target_compile_options(hexdump_test PRIVATE
    -m32
    -Wall
    -Wextra
    -DTESTING
        -fno-builtin
)

# 设置链接选项
set_target_properties(format_string_test PROPERTIES
    LINK_FLAGS "-m32 -static-libstdc++ -static-libgcc -fno-stack-protector"
)

set_target_properties(hexdump_test PROPERTIES
    LINK_FLAGS "-m32 -static-libstdc++ -static-libgcc -fno-stack-protector"
)


# Print all C++ compilation related variables
message(STATUS "C++ Compilation Related Variables:")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS_MINSIZEREL: ${CMAKE_CXX_FLAGS_MINSIZEREL}")
message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
message(STATUS "CMAKE_CXX_STANDARD_REQUIRED: ${CMAKE_CXX_STANDARD_REQUIRED}")
message(STATUS "CMAKE_CXX_EXTENSIONS: ${CMAKE_CXX_EXTENSIONS}")
message(STATUS "CMAKE_CXX_INCLUDE_WHAT_YOU_USE: ${CMAKE_CXX_INCLUDE_WHAT_YOU_USE}")
message(STATUS "CMAKE_CXX_LINK_EXECUTABLE: ${CMAKE_CXX_LINK_EXECUTABLE}")
message(STATUS "CMAKE_CXX_LINK_FLAGS: ${CMAKE_CXX_LINK_FLAGS}")
message(STATUS "CMAKE_CXX_LINK_FLAGS_DEBUG: ${CMAKE_CXX_LINK_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_LINK_FLAGS_RELEASE: ${CMAKE_CXX_LINK_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_LINK_FLAGS_MINSIZEREL: ${CMAKE_CXX_LINK_FLAGS_MINSIZEREL}")
message(STATUS "CMAKE_CXX_LINK_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_LINK_FLAGS_RELWITHDEBINFO}")
message(STATUS "FLAGS: ${FLAGS}")
message(STATUS "LINK_FLAGS: ${LINK_FLAGS}")
message(STATUS "CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_C_LINKER_WRAPPER_FLAG: ${CMAKE_C_LINKER_WRAPPER_FLAG}")
