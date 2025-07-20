cmake_minimum_required(VERSION 3.16)
project(LunaScape)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/libs)

# SDL configuration
set(SDL_SHARED ON CACHE BOOL "Build SDL as shared library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Disable static SDL" FORCE)
set(SDL_IMAGE_SHARED ON CACHE BOOL "Build SDL_image as shared" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Disable SDL tests" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "Disable SDL examples" FORCE)

add_subdirectory(lib/sdl3 EXCLUDE_FROM_ALL)
add_subdirectory(lib/sdl3_image EXCLUDE_FROM_ALL)

# FFmpeg via pkg-config - must be before linking and includes
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
)

# Collect sources
file(GLOB_RECURSE SOURCES "src/*.c")

# GLAD
set(GLAD_SRC ${CMAKE_SOURCE_DIR}/lib/glad/src/glad.c)
set_source_files_properties(${GLAD_SRC} PROPERTIES LANGUAGE C)

# Executable
add_executable(LunaScape ${SOURCES} ${GLAD_SRC})

# Include directories
target_include_directories(LunaScape
    PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/glad/include
        ${CMAKE_SOURCE_DIR}/include
        ${FFMPEG_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(LunaScape
    PRIVATE
        SDL3::SDL3
        SDL3_image::SDL3_image
        ${FFMPEG_LIBRARIES}
)

target_compile_options(LunaScape PRIVATE ${FFMPEG_CFLAGS_OTHER})

# Deployment rules
if(WIN32)
    add_custom_command(TARGET LunaScape POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SDL3::SDL3> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SDL3_image::SDL3_image> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Copying SDL3 DLLs to output directory"
    )
elseif(UNIX)
    set(LIB_DEST ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libs)
    file(MAKE_DIRECTORY ${LIB_DEST})
    add_custom_command(TARGET LunaScape POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SDL3::SDL3> ${LIB_DEST}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SDL3_image::SDL3_image> ${LIB_DEST}
        COMMENT "Copying shared libraries to libs directory"
    )
    set_target_properties(LunaScape PROPERTIES
        INSTALL_RPATH "$ORIGIN/libs"
        BUILD_RPATH "${LIB_DEST}"
    )
endif()
