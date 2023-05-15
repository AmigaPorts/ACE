cmake_minimum_required(VERSION 3.14.0)
project(ACE
	LANGUAGES C ASM
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

# External options
set(ACE_LIBRARY_KIND OBJECT CACHE STRING "Build kind: static library or bunch of .o files. OBJECT allows for more link-time optimizations.")
set(ACE_LIBRARY_KINDS "OBJECT" "STATIC")
set_property(CACHE ACE_LIBRARY_KIND PROPERTY STRINGS ${ACE_LIBRARY_KINDS})
set(ACE_DEBUG OFF CACHE BOOL "Build with ACE-specific debug/safety functionality.")
set(ACE_DEBUG_UAE OFF CACHE BOOL "With ACE_DEBUG enabled, output log to UAE console.")
message(STATUS "[ACE] ACE_LIBRARY_KIND: '${ACE_LIBRARY_KIND}'")
message(STATUS "[ACE] ACE_DEBUG: '${ACE_DEBUG}'")
message(STATUS "[ACE] ACE_DEBUG_UAE: '${ACE_DEBUG_UAE}'")

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

function(getToolPath TOOL_NAME TOOL_VAR)
	# This should be called from other fns - `ACE_DIR` gets usually populated in parent scope
	set(TOOLS_BIN ${ACE_DIR}/tools/bin/)

	if(CMAKE_HOST_WIN32)
		set(TOOL_PATHS "${TOOL_NAME}.exe" "Debug/${TOOL_NAME}.exe" "Release/${TOOL_NAME}.exe")
	else()
		set(TOOL_PATHS ${TOOL_NAME})
	endif()

	foreach(TOOL_CANDIDATE IN LISTS TOOL_PATHS)
		set(${TOOL_VAR} ${TOOLS_BIN}${TOOL_CANDIDATE})
		if(EXISTS "${${TOOL_VAR}}")
			break()
		endif()
		unset(${TOOL_VAR})
	endforeach()

	if(NOT DEFINED ${TOOL_VAR})
		message(FATAL_ERROR "Couldn't find ${TOOL_NAME} in ${TOOLS_BIN}${TOOL_PATHS} - have you built tools?")
	endif()

	# Return value
	set(${TOOL_VAR} ${${TOOL_VAR}} PARENT_SCOPE)
endfunction()

function(convertPalette TARGET PALETTE_IN PALETTE_OUT)
	getToolPath(palette_conv TOOL_PALETTE_CONV)
	add_custom_command(
		OUTPUT ${PALETTE_OUT}
		COMMAND ${TOOL_PALETTE_CONV} ${PALETTE_IN} ${PALETTE_OUT}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${PALETTE_IN}
	)
	target_sources(${TARGET} PUBLIC ${PALETTE_OUT})
endfunction()

function(convertBitmaps)
	getToolPath(bitmap_conv TOOL_BITMAP_CONV)
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
		if("${outPath}" STREQUAL "NONE")
			set(outPath "")
		else()
			toAbsolute(outPath)
		endif()
		if(${maskCount} GREATER 0)
			list(GET convertBitmaps_MASKS ${bitmap_idx} maskPath)
			if("${maskPath}" STREQUAL "NONE")
				set(maskPath "")
			else()
				toAbsolute(maskPath)
			endif()
		endif()

		set(extraFlagsPerFile ${extraFlags})
		if("${outPath} " STREQUAL " ")
			list(APPEND extraFlagsPerFile -no)
		else()
			list(APPEND extraFlagsPerFile -o ${outPath})
		endif()
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
			COMMAND ${TOOL_BITMAP_CONV} ${convertBitmaps_PALETTE} ${bitmapPath} ${extraFlagsPerFile}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS ${convertBitmaps_PALETTE} ${bitmapPath}
		)
		target_sources(${convertBitmaps_TARGET} PUBLIC ${outPath} ${maskPath})
	endforeach()
endfunction()

function(convertFont)
	getToolPath(font_conv TOOL_FONT_CONV)
	cmake_parse_arguments(args "" "TARGET;SOURCE;DESTINATION;FIRST_CHAR" "" ${ARGN})
	toAbsolute(args_SOURCE)
	toAbsolute(args_DESTINATION)
	get_filename_component(ext ${args_DESTINATION} EXT)
	if(ext)
		string(SUBSTRING ${ext} 1 -1 ext)
	else()
		SET(ext "dir")
	endif()

	if(DEFINED args_FIRST_CHAR)
		SET(argsOptional ${argsOptional} -fc ${args_FIRST_CHAR})
	endif()

	add_custom_command(
		OUTPUT ${args_DESTINATION}
		COMMAND ${TOOL_FONT_CONV} ${args_SOURCE} ${ext} -out ${args_DESTINATION} ${argsOptional}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${args_SOURCE}
	)
	target_sources(${args_TARGET} PUBLIC ${args_DESTINATION})
endfunction()

function(transformBitmap)
	getToolPath(bitmap_transform TOOL_BITMAP_TRANSFORM)
	cmake_parse_arguments(args "" "TARGET;SOURCE;DESTINATION" "TRANSFORM" ${ARGN})

	# Make is dumb and randomly has problem with unescaped # or not
	set(argsEscaped "")
	foreach(arg IN LISTS args_TRANSFORM)
		list(APPEND argsEscaped "\"${arg}\"")
	endforeach()

	add_custom_command(
		OUTPUT ${args_DESTINATION}
		COMMAND ${TOOL_BITMAP_TRANSFORM} ${args_SOURCE} ${args_DESTINATION} ${argsEscaped}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${args_SOURCE}
	)
	target_sources(${args_TARGET} PUBLIC ${args_DESTINATION})
endfunction()

function(extractBitmaps)
	cmake_parse_arguments(args "" "TARGET;SOURCE;GENERATED_FILE_LIST" "DESTINATIONS" ${ARGN})
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
		set(generatedFiles "${generatedFiles};${outName}")
	endforeach()

	if(DEFINED args_GENERATED_FILE_LIST)
		SET(${args_GENERATED_FILE_LIST} ${generatedFiles} PARENT_SCOPE)
	endif()
endfunction()

function(convertTileset)
	getToolPath(tileset_conv TOOL_TILESET_CONV)
	cmake_parse_arguments(
		args
		"INTERLEAVED;VARIABLE_HEIGHT"
		"TARGET;SIZE;SOURCE;DESTINATION;HEIGHT;PALETTE"
		"TILE_PATHS" ${ARGN}
	)

	if(DEFINED args_TILE_PATHS)
		foreach(tileNumber ${args_TILE_PATHS})
			set(convDepends "${convDepends};${tileNumber}")
		endforeach()
	else()
		message(FATAL_ERROR "No TILE_PATHS param found")
	endif()

	if(${args_INTERLEAVED})
		SET(argsOptional ${argsOptional} -i)
	endif()

	if(${args_VARIABLE_HEIGHT})
		SET(argsOptional ${argsOptional} -vh)
	endif()

	if(DEFINED args_HEIGHT)
		SET(argsOptional ${argsOptional} -h ${args_HEIGHT})
	endif()

	if(DEFINED args_PALETTE)
		SET(argsOptional ${argsOptional} -plt ${args_PALETTE})
	endif()

	add_custom_command(
		OUTPUT ${args_DESTINATION}
		COMMAND ${TOOL_TILESET_CONV} ${args_SOURCE} ${args_SIZE} ${args_DESTINATION} ${argsOptional}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${convDepends}
	)
	target_sources(${args_TARGET} PUBLIC ${args_DESTINATION})
endfunction()

function(convertAudio)
	getToolPath(audio_conv TOOL_AUDIO_CONV)
	cmake_parse_arguments(args "STRICT" "TARGET;SOURCE;DESTINATION" "" ${ARGN})

	if(${args_STRICT})
		SET(argsOptional -strict)
	endif()

	toAbsolute(args_SOURCE)
	toAbsolute(args_DESTINATION)

	add_custom_command(
		OUTPUT ${args_DESTINATION}
		COMMAND ${TOOL_AUDIO_CONV} ${args_SOURCE} -o ${args_DESTINATION} ${argsOptional}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)
	target_sources(${args_TARGET} PUBLIC ${args_DESTINATION})
endfunction()

function(mergeMods)
	getToolPath(mod_tool TOOL_MOD_TOOL)
	set(oneValArgs SAMPLE_PACK TARGET)
	set(multiValArgs SOURCES DESTINATIONS)
	set(cmdParams "")
	cmake_parse_arguments(
		args "${options}" "${oneValArgs}" "${multiValArgs}" ${ARGN}
	)

	if(NOT ("${args_SAMPLE_PACK} " STREQUAL " "))
		list(APPEND cmdParams -sp ${args_SAMPLE_PACK})
	endif()

	list(LENGTH args_SOURCES srcCount)
	list(LENGTH args_DESTINATIONS dstCount)
	if(NOT ${srcCount} EQUAL ${dstCount})
		message(FATAL_ERROR "[mergeMods] SOURCES count doesn't match DESTINATIONS count")
	endif()

	MATH(EXPR srcCount "${srcCount}-1")
	foreach(mod_idx RANGE ${srcCount})
		list(GET args_SOURCES ${mod_idx} inPath)
		toAbsolute(inPath)
		list(APPEND sourcesAbsolute ${inPath})

		list(GET args_DESTINATIONS ${mod_idx} outPath)
		toAbsolute(outPath)
		list(APPEND destinationsAbsolute ${outPath})

		list(APPEND cmdParams -i ${inPath} -o ${outPath})
	endforeach()

	add_custom_command(
		OUTPUT ${destinationsAbsolute}
		COMMAND ${TOOL_MOD_TOOL} ${cmdParams}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${sourcesAbsolute}
	)
	target_sources(${args_TARGET} PUBLIC ${destinationsAbsolute})
endfunction()
