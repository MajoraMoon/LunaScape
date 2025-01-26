cmake_minimum_required(VERSION 3.15)

project(LunaScape)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD_REQUIRED True)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

file(GLOB_RECURSE SOURCES "src/*.cpp")


add_executable(LunaScape ${SOURCES})

target_include_directories(LunaScape PRIVATE ${CMAKE_SOURCE_DIR}/inlcude)

find_package(SDL3 REQUIRED)
target_link_libraries(LunaScape PRIVATE SDL3::SDL3)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale libswresample)

target_include_directories(LunaScape PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(LunaScape PRIVATE ${FFMPEG_LIBRARIES})

target_compile_options(LunaScape PRIVATE ${FFMPEG_CFLAGS_OTHER})