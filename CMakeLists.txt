cmake_minimum_required(VERSION 3.14.0)
project(ACE
	LANGUAGES C
	DESCRIPTION "Amiga C Engine"
	HOMEPAGE_URL "https://github.com/AmigaPorts/ACE"
)

# TODO: replace with PROJECT_IS_TOP_LEVEL with cmake 3.21+
get_directory_property(hasParent PARENT_DIRECTORY)
if(hasParent)
	# needed for helper fns
	set(ACE_DIR ${CMAKE_CURRENT_LIST_DIR} PARENT_SCOPE)
else()
	set(ACE_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

# Adhere to GNU filesystem layout conventions
include(GNUInstallDirs)

# Lowercase project name for binaries and packaging
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

# Version number in format X.Y.Z
set(VER_X 0)
set(VER_Y 0)
set(VER_Z 1)
set(VER_FULL "${VER_X}.${VER_Y}.${VER_Z}")

if(NOT AMIGA)
	message(SEND_ERROR "[ACE] This project only compiles for Amiga")
endif()

file(GLOB_RECURSE SOURCES src/ace/*.c src/fixmath/*.c)
file(GLOB HEADERS_ACE include/ace/*.h)
file(GLOB HEADERS_ACE_GENERIC include/ace/generic/*.h)
file(GLOB HEADERS_ACE_UTILS include/ace/utils/*.h)
file(GLOB HEADERS_ACE_MANAGERS include/ace/managers/*.h)
file(GLOB HEADERS_ACE_MANAGERS_VP include/ace/managers/viewport/*.h)
file(GLOB HEADERS_FIXMATH include/fixmath/*.h)
set(
	HEADERS
	${HEADERS_ACE} ${HEADERS_ACE_GENERIC} ${HEADERS_ACE_UTILS}
	${HEADERS_ACE_MANAGERS} ${HEADERS_ACE_MANAGERS_VP} ${HEADERS_FIXMATH}
)

include(cmake/ace_config.cmake)

# Linux/other UNIX get a lower-case binary name
set(TARGET_NAME ${PROJECT_NAME_LOWER})

add_library(${TARGET_NAME} ${ACE_LIBRARY_KIND} ${SOURCES} ${HEADERS})

set(CMAKE_C_STANDARD 11)
target_compile_definitions(${TARGET_NAME} PUBLIC AMIGA)
target_compile_options(${TARGET_NAME} PRIVATE $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-Wall -Wextra>)
target_compile_options(${TARGET_NAME} PUBLIC $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:C>>:-fomit-frame-pointer>)

if(ACE_DEBUG)
	target_compile_definitions(${TARGET_NAME} PUBLIC ACE_DEBUG)
endif()

if(ACE_DEBUG_UAE)
	target_compile_definitions(${TARGET_NAME} PUBLIC ACE_DEBUG_UAE)
endif()

if(NOT ACE_BOB_WRAP_Y)
	target_compile_definitions(${TARGET_NAME} PUBLIC ACE_NO_BOB_WRAP_Y)
endif()
if(ACE_BOB_PRISTINE_BUFFER)
	target_compile_definitions(${TARGET_NAME} PUBLIC ACE_BOB_PRISTINE_BUFFER)
endif()
if(ACE_USE_ECS_FEATURES)
	target_compile_definitions(${TARGET_NAME} PUBLIC ACE_USE_ECS_FEATURES)
endif()
target_compile_definitions(${TARGET_NAME} PUBLIC ACE_TILEBUFFER_TILE_TYPE=${ACE_TILEBUFFER_TILE_TYPE})
if(ACE_SCROLLBUFFER_POT_BITMAP_HEIGHT)
target_compile_definitions(${TARGET_NAME} PUBLIC ACE_SCROLLBUFFER_POT_BITMAP_HEIGHT)
endif()
if(ACE_SCROLLBUFFER_ENABLE_SCROLL_X)
target_compile_definitions(${TARGET_NAME} PUBLIC ACE_SCROLLBUFFER_ENABLE_SCROLL_X)
endif()
if(ACE_SCROLLBUFFER_ENABLE_SCROLL_Y)
target_compile_definitions(${TARGET_NAME} PUBLIC ACE_SCROLLBUFFER_ENABLE_SCROLL_Y)
endif()
target_compile_definitions(${TARGET_NAME} PUBLIC ACE_SCROLLBUFFER_X_MARGIN_SIZE=${ACE_SCROLLBUFFER_X_MARGIN_SIZE})
target_compile_definitions(${TARGET_NAME} PUBLIC ACE_SCROLLBUFFER_Y_MARGIN_SIZE=${ACE_SCROLLBUFFER_Y_MARGIN_SIZE})

if(M68K_COMPILER MATCHES "Bartman")
	include(cmake/CPM.cmake)
	CPMAddPackage(
		NAME bartman_gcc_support
		GITHUB_REPOSITORY AmigaPorts/bartman_gcc_support
		GIT_TAG latest
	)
	# Workaround for nested OBJECT library
	# https://stackoverflow.com/questions/71040175
	target_link_libraries(${TARGET_NAME} PUBLIC bartman_gcc_support "$<TARGET_OBJECTS:bartman_gcc_support>")
endif()
if(M68K_COMPILER MATCHES "Bartman" OR USE_MINI_STD)
	target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/mini_std)
	file(GLOB SOURCES_MINI_STD src/mini_std/*.c)
	file(GLOB HEADERS_MINI_STD include/mini_std/*.h)
	target_compile_definitions(${TARGET_NAME} PRIVATE PRINTF_DISABLE_SUPPORT_FLOAT PRINTF_DISABLE_SUPPORT_LONG_LONG)
	target_sources(${TARGET_NAME} PRIVATE ${SOURCES_MINI_STD} ${HEADERS_MINI_STD})
	message(STATUS "[ACE] Using mini std")
endif()

target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

include(cmake/ace_install.cmake)
include(cmake/ace_functions.cmake)
