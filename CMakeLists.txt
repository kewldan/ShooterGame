cmake_minimum_required(VERSION 3.23)
project(ShooterGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR}/libraries/glad/include)
#include_directories(${PROJECT_SOURCE_DIR}/libraries/stb)
include_directories(${PROJECT_SOURCE_DIR}/libraries/plog/include)
include_directories(${PROJECT_SOURCE_DIR}/libraries/glfw/include)
include_directories(${PROJECT_SOURCE_DIR}/libraries/loader/include)
include_directories(${PROJECT_SOURCE_DIR}/libraries/glm)

link_directories(${PROJECT_SOURCE_DIR}/libraries/glfw/lib-mingw-w64)

add_executable(ShooterGame libraries/glad/src/gl.c main.cpp Shader.cpp Shader.h Window.cpp Window.h Mesh.cpp Mesh.h Model.cpp Model.h Camera.cpp Camera.h)

target_link_libraries(ShooterGame glfw3 opengl32 gdi32)
