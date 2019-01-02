cmake_minimum_required(VERSION 2.8.5)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
project(ACE_tools)

# Adhere to GNU filesystem layout conventions
include(GNUInstallDirs)

file(GLOB_RECURSE FONT_CONV_src font_conv/*.cpp font_conv/*.c)
file(GLOB_RECURSE FONT_CONV_hdr font_conv/*.hpp font_conv/*.h)

add_executable(font_conv ${FONT_CONV_src} ${FONT_CONV_hdr})

#fmt
find_package(fmt REQUIRED)
include_directories(${fmt_INCLUDE_DIRS})
target_link_libraries(font_conv fmt::fmt)
