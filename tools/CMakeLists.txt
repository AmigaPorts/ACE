cmake_minimum_required(VERSION 2.8.5)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
project(ACE_tools)

# Adhere to GNU filesystem layout conventions
include(GNUInstallDirs)

file(GLOB FONT_CONV_src
	src/font_conv.cpp src/tools/*.cpp src/common/*.cpp src/common/*.c
)
file(GLOB_RECURSE FONT_CONV_hdr
	src/tools/font_converter.h src/common/*.h src/common/*.hpp
)

add_executable(font_conv ${FONT_CONV_src} ${FONT_CONV_hdr})

#fmt
find_package(fmt REQUIRED)
include_directories(${fmt_INCLUDE_DIRS})
target_link_libraries(font_conv fmt::fmt)
