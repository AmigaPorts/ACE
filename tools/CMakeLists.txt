cmake_minimum_required(VERSION 3.0.0)
# Generator expression skips Debug/Release/... directories for MSVC
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/$<0:>)
project(ACE_tools)

# Adhere to GNU filesystem layout conventions
include(GNUInstallDirs)
set (CMAKE_CXX_STANDARD 20)

# MSVC needs a few defines to be GNU compatible
if(MSVC)
    add_compile_definitions(_USE_MATH_DEFINES=1 NOMINMAX=1)
endif()

# Common
file(GLOB COMMON_src src/common/*.cpp src/common/*.c)
file(GLOB_RECURSE COMMON_hdr src/common/*.h src/common/*.hpp)
add_library(common STATIC ${COMMON_src} ${COMMON_hdr})
if(NOT MSVC)
	target_compile_options(common PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-register>)
	target_link_libraries(common -static)
endif()

#fmt
add_subdirectory(fmt)
target_include_directories(fmt PUBLIC ${fmt_INCLUDE_DIRS})
target_link_libraries(common fmt::fmt)

# App-related
file(GLOB FONT_CONV_src src/font_conv.cpp)
file(GLOB PALETTE_CONV_src src/palette_conv.cpp)
file(GLOB BITMAP_TRANSFORM_src src/bitmap_transform.cpp)
file(GLOB TILESET_CONV_src src/tileset_conv.cpp)
file(GLOB BITMAP_CONV_src src/bitmap_conv.cpp)
file(GLOB AUDIO_CONV_src src/audio_conv.cpp)
file(GLOB MOD_TOOL_src src/mod_tool.cpp)

add_executable(font_conv ${FONT_CONV_src})
add_executable(palette_conv ${PALETTE_CONV_src})
add_executable(bitmap_transform ${BITMAP_TRANSFORM_src})
add_executable(tileset_conv ${TILESET_CONV_src})
add_executable(bitmap_conv ${BITMAP_CONV_src})
add_executable(audio_conv ${AUDIO_CONV_src})
add_executable(mod_tool ${MOD_TOOL_src})

target_link_libraries(font_conv common)
target_link_libraries(palette_conv common)
target_link_libraries(bitmap_transform common)
target_link_libraries(tileset_conv common)
target_link_libraries(bitmap_conv common)
target_link_libraries(audio_conv common)
target_link_libraries(mod_tool common)
