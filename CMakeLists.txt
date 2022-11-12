cmake_minimum_required(VERSION 3.23)
project(ShooterGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${PROJECT_SOURCE_DIR}/libraries/glad/include)
include_directories(${PROJECT_SOURCE_DIR}/libraries/stb)
include_directories(${PROJECT_SOURCE_DIR}/libraries/plog/include)
include_directories(${PROJECT_SOURCE_DIR}/libraries/glfw/include)
include_directories(${PROJECT_SOURCE_DIR}/libraries/md5/include)
include_directories(${PROJECT_SOURCE_DIR}/libraries/glm)
include_directories(${PROJECT_SOURCE_DIR}/imgui)

link_directories(${PROJECT_SOURCE_DIR}/libraries/glfw/lib-vc2022)

set(assimp_DIR E:\\dev\\vcpkg\\installed\\x64-windows\\share\\assimp)

find_package(ReactPhysics3D REQUIRED)
find_package(assimp REQUIRED)

link_directories(E:\\dev\\vcpkg\\installed\\x64-windows\\lib)

add_executable(ShooterGame
        libraries/glad/src/gl.c
        main.cpp
        Shader.cpp Shader.h
        Window.cpp Window.h
        MyMesh.cpp MyMesh.h
        Model.cpp Model.h
        Camera.cpp Camera.h
        Texture.cpp Texture.h
        ShadowsCaster.cpp ShadowsCaster.h

        imgui/imconfig.h
        imgui/imgui_draw.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_internal.h
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_glfw.h
        imgui/backends/imgui_impl_opengl3.cpp imgui/backends/imgui_impl_opengl3.h
        imgui/imgui.cpp imgui/imgui.h

        HUD.cpp HUD.h
        MyProfiler.cpp MyProfiler.h
        Client.cpp Client.h
 "libraries/md5/include/md5.cpp")

target_link_libraries(ShooterGame glfw3 opengl32 gdi32 ReactPhysics3D::ReactPhysics3D Ws2_32 assimp::assimp)