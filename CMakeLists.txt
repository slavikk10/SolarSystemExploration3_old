cmake_minimum_required(VERSION 3.10)
set(CMAKE_OSX_ARCHITECTURES "arm64")
project(sse3)

execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE MACOS_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CMAKE_OSX_SYSROOT "${MACOS_SDK_PATH}")
set(CMAKE_XCODE_VERSION 16.4)
add_compile_options(-Wno-nullability-completeness)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED)

add_executable(sse3  main.cpp src/glad.c stb_image.cpp include/)

include_directories(/Users/vyacheslav/.brew/opt/glfw/include)
include_directories(/Users/vyacheslav/.brew/opt/assimp/include)
include_directories(/Users/vyacheslav/.brew/opt/freetype/include)
include_directories(/Users/vyacheslav/SSE3_OpenGL/include)

target_link_libraries(sse3 /Users/vyacheslav/.brew/opt/glfw/lib/libglfw.3.dylib)
target_link_libraries(sse3 /Users/vyacheslav/.brew/opt/assimp/lib/libassimp.5.4.3.dylib)
target_link_libraries(sse3 /Users/vyacheslav/.brew/opt/freetype/lib/libfreetype.6.dylib)

target_link_libraries(sse3 OpenGL::GL)
