cmake_minimum_required(VERSION 3.23)
project(ShooterGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(LIBRARIES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/libraries)

include(${LIBRARIES_PATH}/imgui/import.cmake)

include_directories(${LIBRARIES_PATH}/glad/include)
include_directories(${LIBRARIES_PATH}/stb/include)
include_directories(${LIBRARIES_PATH}/plog/include)
include_directories(${LIBRARIES_PATH}/glfw/include)
include_directories(${LIBRARIES_PATH}/profiler/include)
include_directories(${LIBRARIES_PATH}/glm/include)
include_directories(${LIBRARIES_PATH}/ReactPhysics3D/include)
include_directories(${LIBRARIES_PATH}/imgui)

find_package(assimp REQUIRED)

link_directories("E:\\dev\\vcpkg\\installed\\x64-windows\\lib")
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libraries/ReactPhysics3D/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libraries/glfw/lib-vc2022)

add_executable(${PROJECT_NAME}
        libraries/glad/src/gl.c
        libraries/profiler/include/Remotery.c libraries/profiler/include/Remotery.h
        ${IMGUI_SOURCES}

        main.cpp
        Shader.cpp Shader.h
        Window.cpp Window.h
        MyMesh.cpp MyMesh.h
        Model.cpp Model.h
        Camera.cpp Camera.h
        Texture.cpp Texture.h
        ShadowsCaster.cpp ShadowsCaster.h
        HUD.cpp HUD.h
        Client.cpp Client.h
        Chat.cpp Chat.h
 "Skybox.h" "Skybox.cpp" "Minimap.h" "Minimap.cpp" "GBuffer.h" "GBuffer.cpp")

set(GAME_LIBRARIES glfw3 opengl32 gdi32 Ws2_32 assimp::assimp)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME} ${GAME_LIBRARIES} reactphysics3dd)
else()
    target_link_libraries(${PROJECT_NAME} ${GAME_LIBRARIES} reactphysics3d)
endif()