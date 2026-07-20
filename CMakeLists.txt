cmake_minimum_required(VERSION 3.20)


set(NAME "ImmersiveFastTravelEncountersSSE")
set(VERSION 1.0.0)


# ---- Options ----

option(COPY_BUILD "Copy the build output to the Skyrim directory." OFF)
option(ENABLE_SKYRIM_SE "Enable support for Skyrim SE in the dynamic runtime feature." ON)
option(ENABLE_SKYRIM_AE "Enable support for Skyrim AE in the dynamic runtime feature." ON)
option(ENABLE_SKYRIM_VR "Enable support for Skyrim VR in the dynamic runtime feature." ON)
option(BUILD_TESTS "Enable building of the unit tests." OFF)

# ---- Cache build vars ----

macro(set_from_environment VARIABLE)
	if(NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
		set(${VARIABLE} $ENV{${VARIABLE}})
	endif()
endmacro()

macro(find_commonlib_path)
	if(CommonLibName AND NOT ${CommonLibName} STREQUAL "")
		# Check extern
		find_path(CommonLibPath
				include/REL/Relocation.h
				PATHS extern/${CommonLibName}
				extern/${CommonLibName}/${CommonLibName}
		)
		if(${CommonLibName} STREQUAL "CommonLibName-NOTFOUND")
			# Check path
			set_from_environment(${CommonLibName}Path)
			set(CommonLibName ${${CommonLibName}Path})
		endif()
	endif()
endmacro()

set_from_environment(VCPKG_ROOT)
if(DEFINED VCPKG_ROOT)
	set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
	set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
else()
	message(
		WARNING
		"Variable VCPKG_ROOT is not set. Continuing without vcpkg."
	)
endif()

set_from_environment(SkyrimPath)
set(CommonLibName "CommonLibSSE-NG")
find_commonlib_path()

if(DEFINED SkyrimPath)
	message(
		STATUS
		"Building for Skyrim at ${SkyrimPath} with ${CommonLibName} at ${CommonLibPath}."
	)
else()
	message(
		STATUS
		"Building for Skyrim with ${CommonLibName} at ${CommonLibPath}."
	)
endif()

set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")

# ---- Project ----

project(
	${NAME}
	VERSION ${VERSION}
	LANGUAGES CXX
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in
	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
	@ONLY
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/version.rc
	@ONLY
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(
		FATAL_ERROR
			"In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
	)
endif()

# ---- Globals ----

#[[
add_compile_definitions(
	SKSE_SUPPORT_XBYAK
)
#]]

if(MSVC)
	if(NOT ${CMAKE_GENERATOR} STREQUAL "Ninja")
		add_compile_options(
			/MP	# Build with Multiple Processes
		)
	endif()
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

set(Boost_USE_STATIC_LIBS ON)

# ---- Dependencies ----

if(DEFINED CommonLibPath AND NOT ${CommonLibPath} STREQUAL "" AND IS_DIRECTORY ${CommonLibPath})
	add_subdirectory(${CommonLibPath} ${CommonLibName})
else()
	message(
		FATAL_ERROR
		"Variable ${CommonLibName}Path is not set or in extern/."
	)
endif()

# ---- Add source files ----

file(GLOB_RECURSE HEADER_FILES
	CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

file(GLOB_RECURSE SOURCE_FILES
	CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

source_group(
	TREE
		${CMAKE_CURRENT_SOURCE_DIR}
	FILES
		${HEADER_FILES}
		${SOURCE_FILES}
)

source_group(
	TREE
		${CMAKE_CURRENT_BINARY_DIR}
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
)

# ---- Create DLL ----

#find_package(CommonLibSSE REQUIRED)

#add_commonlibsse_plugin(${PROJECT_NAME}
#    SOURCES ${HEADER_FILES} ${SOURCE_FILES})

add_library(
	${PROJECT_NAME}
	SHARED
	${HEADER_FILES}
	${SOURCE_FILES}
	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h
	${CMAKE_CURRENT_BINARY_DIR}/version.rc
	.clang-format
	.editorconfig
)

target_compile_features(
	${PROJECT_NAME}
	PRIVATE
		cxx_std_23
)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
		_UNICODE
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
		CommonLibSSE::CommonLibSSE
)

target_precompile_headers(
	${PROJECT_NAME}
	PRIVATE
		src/PCH.h
)

if(MSVC)
	target_compile_options(
		${PROJECT_NAME}
		PRIVATE
			/sdl	# Enable Additional Security Checks
			/utf-8	# Set Source and Executable character sets to UTF-8
			/Zi	# Debug Information Format

			/permissive-	# Standards conformance
			/Zc:preprocessor	# Enable preprocessor conformance mode

			/wd4200 # nonstandard extension used : zero-sized array in struct/union

			"$<$<CONFIG:DEBUG>:>"
			"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
	)

	target_link_options(
		${PROJECT_NAME}
		PRIVATE
			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)
endif()

# ---- Post build ----

if(COPY_BUILD)
	if(DEFINED SkyrimPath)
		add_custom_command(
			TARGET ${PROJECT_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${SkyrimPath}/Data/SKSE/Plugins/
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> ${SkyrimPath}/Data/SKSE/Plugins/
		)
	else()
		message(
			WARNING
			"Variable ${SkyrimPath} is not defined. Skipping post-build copy command."
		)
	endif()
endif()
