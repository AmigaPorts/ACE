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
	set(options CONVERT_COLORS)
	set(oneValArgs)
	set(multiValArgs)
	cmake_parse_arguments(
		convertPalette "${options}" "${oneValArgs}" "${multiValArgs}" ${ARGN}
	)

	if(${convertPalette_CONVERT_COLORS})
		list(APPEND extraFlags "-cc")
	endif()

	add_custom_command(
		OUTPUT ${PALETTE_OUT}
		COMMAND ${TOOL_PALETTE_CONV} ${PALETTE_IN} ${PALETTE_OUT} ${extraFlags}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${PALETTE_IN}
	)
	target_sources(${TARGET} PUBLIC ${PALETTE_OUT})
endfunction()

function(convertBitmaps)
	getToolPath(bitmap_conv TOOL_BITMAP_CONV)
	set(options INTERLEAVED EHB)
	set(oneValArgs TARGET PALETTE MASK_COLOR)
	set(multiValArgs SOURCES DESTINATIONS MASKS)
	cmake_parse_arguments(
		convertBitmaps "${options}" "${oneValArgs}" "${multiValArgs}" ${ARGN}
	)

	if(${convertBitmaps_EHB})
		list(APPEND extraFlags "-ehb")
	endif()

	if(${convertBitmaps_INTERLEAVED})
		list(APPEND extraFlags "-i")
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
		"TARGET;SIZE;SOURCE;DESTINATION;HEIGHT;PALETTE;COLUMN_WIDTH"
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

	if(DEFINED args_COLUMN_WIDTH)
		SET(argsOptional ${argsOptional} -cw ${args_COLUMN_WIDTH})
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
	cmake_parse_arguments(
		args
		"STRICT;PTPLAYER;NORMALIZE;COMPRESS;"
		"TARGET;SOURCE;DESTINATION;PAD_BYTES;DIVIDE_AMPLITUDE;CHECK_DIVIDED_AMPLITUDE"
		"" ${ARGN}
	)

	if(${args_STRICT})
		set(argsOptional ${argsOptional} -strict)
	endif()
	if(${args_PTPLAYER})
		set(argsOptional ${argsOptional} -fpt)
	endif()
	if(${args_NORMALIZE})
		set(argsOptional ${argsOptional} -n)
	endif()
	if(${args_COMPRESS})
		set(argsOptional ${argsOptional} -c)
	endif()
	if(${args_PAD_BYTES})
		set(argsOptional ${argsOptional} -pad ${args_PAD_BYTES})
	endif()

	if(${args_CHECK_DIVIDED_AMPLITUDE})
		set(argsOptional ${argsOptional} -cd ${args_CHECK_DIVIDED_AMPLITUDE})
	elseif(${args_DIVIDE_AMPLITUDE})
		set(argsOptional ${argsOptional} -d ${args_DIVIDE_AMPLITUDE})
	endif()

	toAbsolute(args_SOURCE)
	toAbsolute(args_DESTINATION)

	add_custom_command(
		OUTPUT ${args_DESTINATION}
		COMMAND ${TOOL_AUDIO_CONV} ${args_SOURCE} -o ${args_DESTINATION} ${argsOptional}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${args_SOURCE}
	)
	target_sources(${args_TARGET} PUBLIC ${args_DESTINATION})
endfunction()

function(mergeMods)
	getToolPath(mod_tool TOOL_MOD_TOOL)
	set(cmdParams "")
	cmake_parse_arguments(
		args
		"COMPRESS"
		"SAMPLE_PACK;TARGET"
		"SOURCES;DESTINATIONS" ${ARGN}
	)

	if(NOT ("${args_SAMPLE_PACK} " STREQUAL " "))
		list(APPEND cmdParams -sp ${args_SAMPLE_PACK})
	endif()

	if(${args_COMPRESS})
		list(APPEND cmdParams -c)
	endif()

	list(LENGTH args_SOURCES srcCount)
	list(LENGTH args_DESTINATIONS dstCount)
	if(NOT ${srcCount} EQUAL ${dstCount})
		message(FATAL_ERROR "[mergeMods] SOURCES count ${srcCount} doesn't match DESTINATIONS count ${dstCount}")
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

function(packDirectory)
	getToolPath(pak_tool TOOL_PAK_TOOL)
	cmake_parse_arguments(
		args
		"COMPRESS"
		"SOURCE_DIR;DEST_FILE;TARGET;REORDER_FILE"
		""
		${ARGN}
	)

	toAbsolute(args_SOURCE_DIR)
	toAbsolute(args_DEST_FILE)
	FILE(GLOB_RECURSE sourceDirFiles "${args_SOURCE_DIR}/*")

	if(${args_COMPRESS})
		set(argsOptional ${argsOptional} -c)
	endif()
	if(NOT "${args_REORDER_FILE} " STREQUAL " ")
		set(argsOptional ${argsOptional} -r ${args_REORDER_FILE})
	endif()

	add_custom_command(
		OUTPUT ${args_DEST_FILE}
		COMMAND ${TOOL_PAK_TOOL} ${args_SOURCE_DIR} ${args_DEST_FILE} ${argsOptional}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		DEPENDS ${sourceDirFiles}
	)
	target_sources(${args_TARGET} PUBLIC ${args_DEST_FILE})
endfunction()
