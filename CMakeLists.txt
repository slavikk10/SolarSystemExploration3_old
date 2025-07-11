cmake_minimum_required(VERSION 3.10)
set(CMAKE_OSX_ARCHITECTURES "arm64")
project(sse3)

set(CMAKE_OSX_SYSROOT "/Users/vyacheslav/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.5.sdk" CACHE STRING "macOS SDK path")
set(CMAKE_XCODE_VERSION 15.5)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED)

add_executable(sse3  main.cpp src/glad.c stb_image.cpp include/)

include_directories(/Users/vyacheslav/.brew/opt/glfw/include)
include_directories(/Users/vyacheslav/.brew/opt/assimp/include)
include_directories(/Users/vyacheslav/SSE3_OpenGL/include)

target_link_libraries(sse3 /Users/vyacheslav/.brew/opt/glfw/lib/libglfw.3.dylib)
target_link_libraries(sse3 /Users/vyacheslav/.brew/opt/assimp/lib/libassimp.5.4.3.dylib)

target_link_libraries(sse3 OpenGL::GL)
