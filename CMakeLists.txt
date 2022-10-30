cmake_minimum_required(VERSION 3.23)
project(OpenGL)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static")

include_directories(${PROJECT_SOURCE_DIR}/glad/include)
include_directories(${PROJECT_SOURCE_DIR}/linmath/include)
include_directories(${PROJECT_SOURCE_DIR}/stb)
include_directories(${PROJECT_SOURCE_DIR}/plog/include)
include_directories(${PROJECT_SOURCE_DIR}/glfw/include)

link_directories(${PROJECT_SOURCE_DIR}/glfw/lib-mingw-w64)

add_executable(OpenGL main.cpp glad/src/gl.c Shader.cpp Shader.h Window.cpp Window.h Mesh.cpp Mesh.h)

target_link_libraries(OpenGL glfw3 opengl32 gdi32)
