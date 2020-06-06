cmake_minimum_required(VERSION 2.8.5)
project(ACE LANGUAGES C ASM)
set(ACE_DIR ${CMAKE_CURRENT_LIST_DIR} PARENT_SCOPE) # needed for helper fns

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

# Linux/other UNIX get a lower-case binary name
set(TARGET_NAME ${PROJECT_NAME_LOWER})
add_library(${TARGET_NAME} ${SOURCES} ${HEADERS})

set(CMAKE_C_STANDARD 11)
target_compile_definitions(${TARGET_NAME} PUBLIC AMIGA)
target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra)
target_compile_options(${TARGET_NAME} PUBLIC -fomit-frame-pointer)
if(ACE_DEBUG)
	# Add ACE debugging features to release build
	target_compile_definitions(${TARGET_NAME} PUBLIC ACE_DEBUG)
endif()
if(M68K_COMPILER MATCHES "Bartman")
	target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/bartman)
	file(GLOB SOURCES_BARTMAN src/mini_std/*.c src/bartman/*.c src/bartman/*.s)
	file(GLOB HEADERS_BARTMAN include/mini_std/*.h include/bartman/*.h)
	target_compile_definitions(${TARGET_NAME} PRIVATE PRINTF_DISABLE_SUPPORT_FLOAT PRINTF_DISABLE_SUPPORT_LONG_LONG)
	target_sources(${TARGET_NAME} PRIVATE ${SOURCES_BARTMAN} ${HEADERS_BARTMAN})
endif()
if(M68K_COMPILER MATCHES "Bartman" OR USE_MINI_STD)
	target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/mini_std)
	file(GLOB SOURCES_MINI_STD src/mini_std/*.c)
	file(GLOB HEADERS_MINI_STD include/mini_std/*.h)
	target_sources(${TARGET_NAME} PRIVATE ${SOURCES_MINI_STD} ${HEADERS_MINI_STD})
	message(STATUS "[ACE] Using mini std")
endif()

target_include_directories(${TARGET_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

#----------------------------------------------------------------------- INSTALL

install(TARGETS ${TARGET_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${HEADERS_ACE} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ace)
install(FILES ${HEADERS_ACE_GENERIC} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ace/generic)
install(FILES ${HEADERS_ACE_UTILS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ace/utils)
install(FILES ${HEADERS_ACE_MANAGERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ace/managers)
install(FILES ${HEADERS_ACE_MANAGERS_VP} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ace/managers/viewport)
install(FILES ${HEADERS_FIXMATH} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fixmath)

#--------------------------------------------------------------------- FUNCTIONS

function(toAbsolute PATH_IN)
	if(NOT IS_ABSOLUTE ${${PATH_IN}})
		set(${PATH_IN} "${CMAKE_CURRENT_SOURCE_DIR}/${${PATH_IN}}" PARENT_SCOPE)
	endif()
endfunction()

function(convertPalette TARGET PALETTE_IN PALETTE_OUT)
	add_custom_command(
		OUTPUT ${PALETTE_OUT}
		COMMAND ${ACE_DIR}/tools/bin/palette_conv ${PALETTE_IN} ${PALETTE_OUT}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${PALETTE_IN}
	)
	target_sources(${TARGET} PUBLIC ${PALETTE_OUT})
endfunction()

function(convertBitmaps)
	set(options INTERLEAVED)
	set(oneValArgs TARGET PALETTE MASK_COLOR)
	set(multiValArgs SOURCES DESTINATIONS MASKS)
	cmake_parse_arguments(
		convertBitmaps "${options}" "${oneValArgs}" "${multiValArgs}" ${ARGN}
	)

	if(${convertBitmaps_INTERLEAVED})
		set(extraFlags -i)
	endif()

	list(LENGTH convertBitmaps_SOURCES srcCount)
	list(LENGTH convertBitmaps_DESTINATIONS dstCount)
	list(LENGTH convertBitmaps_MASKS maskCount)
	if(NOT ${srcCount} EQUAL ${dstCount})
		message(FATAL_ERROR "[convertBitmaps] SOURCES count doesn't match DESTINATIONS count")
	endif()
	if(${maskCount} AND NOT ${maskCount} EQUAL ${srcCount})
		message(FATAL_ERROR "[convertBitmaps] MASKS count doesn't match SOURCES count")
	endif()
	if("${convertBitmaps_MASK_COLOR} " STREQUAL " ")
		if(${maskCount} GREATER 0)
			message(FATAL_ERROR "[convertBitmaps] MASK_COLOR unspecified")
		endif()
	endif()

	MATH(EXPR srcCount "${srcCount}-1")
	foreach(bitmap_idx RANGE ${srcCount}) # /path/file.png
		list(GET convertBitmaps_SOURCES ${bitmap_idx} bitmapPath)
		toAbsolute(bitmapPath)
		list(GET convertBitmaps_DESTINATIONS ${bitmap_idx} outPath)
		toAbsolute(outPath)
		if(${maskCount} GREATER 0)
			list(GET convertBitmaps_MASKS ${bitmap_idx} maskPath)
			if("${maskPath}" STREQUAL "NONE")
				set(maskPath "")
			else()
				toAbsolute(maskPath)
			endif()
		endif()

		set(extraFlagsPerFile ${extraFlags})
		if(NOT "${convertBitmaps_MASK_COLOR} " STREQUAL " ")
			list(APPEND extraFlagsPerFile -mc "\"${convertBitmaps_MASK_COLOR}\"")
			if("${maskPath} " STREQUAL " ")
				list(APPEND extraFlagsPerFile -nmo)
			else()
				list(APPEND extraFlagsPerFile -mf ${maskPath})
			endif()
		endif()
		add_custom_command(
			OUTPUT ${outPath} ${maskPath}
			COMMAND ${ACE_DIR}/tools/bin/bitmap_conv ${convertBitmaps_PALETTE} ${bitmapPath} -o ${outPath} ${extraFlagsPerFile}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS ${convertBitmaps_PALETTE} ${bitmapPath}
		)
		target_sources(${convertBitmaps_TARGET} PUBLIC ${outPath} ${maskPath})
	endforeach()
endfunction()

function(convertFont TARGET FONT_IN FONT_OUT)
	toAbsolute(FONT_IN)
	toAbsolute(FONT_OUT)
	get_filename_component(ext ${FONT_OUT} EXT)
	string(SUBSTRING ${ext} 1 -1 ext)
	add_custom_command(
		OUTPUT ${FONT_OUT}
		COMMAND ${ACE_DIR}/tools/bin/font_conv ${FONT_IN} ${ext} -out ${FONT_OUT}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${FONT_IN}
	)
	target_sources(${TARGET} PUBLIC ${FONT_OUT})
endfunction()

function(transformBitmap)
	cmake_parse_arguments(args "" "TARGET;SOURCE;DESTINATION" "TRANSFORM" ${ARGN})

	# Make is dumb and randomly has problem with unescaped # or not
	set(argsEscaped "")
	foreach(arg IN LISTS args_TRANSFORM)
		list(APPEND argsEscaped "\"${arg}\"")
	endforeach()

	add_custom_command(
		OUTPUT ${args_DESTINATION}
		COMMAND ${ACE_DIR}/tools/bin/bitmap_transform ${args_SOURCE} ${args_DESTINATION} ${argsEscaped}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${args_SOURCE}
	)
	target_sources(${args_TARGET} PUBLIC ${args_DESTINATION})
endfunction()

function(extractBitmaps)
	cmake_parse_arguments(args "" "TARGET;SOURCE" "DESTINATIONS" ${ARGN})
	list(LENGTH args_DESTINATIONS destArgCount)
	MATH(EXPR destCount "${destArgCount} / 5 - 1")
	foreach(destIdx RANGE ${destCount})
		MATH(EXPR idxOutName "5 * ${destIdx} + 0")
		MATH(EXPR idxX "5 * ${destIdx} + 1")
		MATH(EXPR idxY "5 * ${destIdx} + 2")
		MATH(EXPR idxWidth "5 * ${destIdx} + 3")
		MATH(EXPR idxHeight "5 * ${destIdx} + 4")

		list(GET args_DESTINATIONS ${idxOutName} outName)
		list(GET args_DESTINATIONS ${idxX} X)
		list(GET args_DESTINATIONS ${idxY} Y)
		list(GET args_DESTINATIONS ${idxWidth} width)
		list(GET args_DESTINATIONS ${idxHeight} height)
		transformBitmap(
			TARGET ${args_TARGET} SOURCE ${args_SOURCE} DESTINATION ${outName}
			TRANSFORM -extract ${X} ${Y} ${width} ${height}
		)
	endforeach()
endfunction()

function(convertTileset)
	cmake_parse_arguments(
		args "" "TARGET;SIZE;SOURCE;DESTINATION;HEIGHT" "TILES" ${ARGN}
	)

	list(LENGTH args_TILES tilesCount)
	foreach(tileNumber ${args_TILES})
		set(convDepends "${convDepends};${args_SOURCE}/${tileNumber}")
	endforeach()

	if(DEFINED args_HEIGHT)
		SET(argsOptional ${argsOptional} -h ${args_HEIGHT})
	endif()

	add_custom_command(
		OUTPUT ${args_DESTINATION}
		COMMAND ${ACE_DIR}/tools/bin/tileset_conv ${args_SOURCE} ${args_SIZE} ${args_DESTINATION} ${argsOptional}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${convDepends}
	)
	target_sources(${args_TARGET} PUBLIC ${args_DESTINATION})
endfunction()
