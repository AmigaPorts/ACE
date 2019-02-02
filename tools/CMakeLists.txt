cmake_minimum_required(VERSION 2.8.5)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
project(ACE_tools)

# Adhere to GNU filesystem layout conventions
include(GNUInstallDirs)
set (CMAKE_CXX_STANDARD 17)

file(GLOB FONT_CONV_src src/font_conv.cpp)
file(GLOB PALETTE_CONV_src src/palette_conv.cpp)
file(GLOB COMMON_src src/common/*.cpp src/common/*.c)
file(GLOB_RECURSE COMMON_hdr src/common/*.h src/common/*.hpp)

add_executable(font_conv ${FONT_CONV_src} ${COMMON_src} ${COMMON_hdr})
add_executable(palette_conv ${PALETTE_CONV_src} ${COMMON_src} ${COMMON_hdr})

#fmt
find_package(fmt REQUIRED)
include_directories(${fmt_INCLUDE_DIRS})
target_link_libraries(font_conv fmt::fmt)
target_link_libraries(palette_conv fmt::fmt)
