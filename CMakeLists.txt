cmake_minimum_required(VERSION 3.15)

project(LunaScape)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

file(GLOB_RECURSE SOURCES "src/*.c")

set(GLAD_SRC ${CMAKE_SOURCE_DIR}/lib/glad/src/glad.c)

add_executable(LunaScape ${SOURCES} ${GLAD_SRC})

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(/usr/local/include/SDL3_image)
include_directories(${CMAKE_SOURCE_DIR}/lib/glad/include)




# When I'm using "SDL3::SDL3" for target_link_libraries, 
# I get an undefined reference to `SDL_WaitAndAcquireGPUSwapchainTexture'.
# Arch repos are not that fast, SDL3_image is not ther yet lol so did it manually
find_package(SDL3 REQUIRED)
target_link_libraries(LunaScape PRIVATE /usr/lib/libSDL3.so)
target_link_libraries(LunaScape PRIVATE /usr/local/lib/libSDL3_image.so)



find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale libswresample)

target_include_directories(LunaScape PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(LunaScape PRIVATE ${FFMPEG_LIBRARIES})

target_compile_options(LunaScape PRIVATE ${FFMPEG_CFLAGS_OTHER})


pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
target_include_directories(LunaScape PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
target_link_libraries(LunaScape PRIVATE ${PORTAUDIO_LIBRARIES})
target_compile_options(LunaScape PRIVATE ${PORTAUDIO_CFLAGS_OTHER})