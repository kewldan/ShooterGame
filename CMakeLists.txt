cmake_minimum_required(VERSION 3.23)
project(ShooterGame)

set(NETWORKING_PORT 23403)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBRARIES_PATH "E:/Projects/libraries")

include(${LIBRARIES_PATH}/imgui/import.cmake)

include_directories(
        ${PROJECT_SOURCE_DIR}/include

        ${LIBRARIES_PATH}/glad/debug/include
        ${LIBRARIES_PATH}/stb/include
        ${LIBRARIES_PATH}/plog/include
        ${LIBRARIES_PATH}/glfw/include
        ${LIBRARIES_PATH}/glm/include
        ${LIBRARIES_PATH}/zlib/
        ${LIBRARIES_PATH}/ReactPhysics3D/include
        ${LIBRARIES_PATH}/objl
        ${LIBRARIES_PATH}/imgui
        ${LIBRARIES_PATH}/engine/include
)

link_directories(${LIBRARIES_PATH}/ReactPhysics3D/lib)
link_directories(${LIBRARIES_PATH}/glfw/lib-vc2022)
link_directories(${LIBRARIES_PATH}/engine/lib)
link_directories(${LIBRARIES_PATH}/zlib/lib)

# Settings

add_definitions(-DNETWORKING_PORT=${NETWORKING_PORT})

##########

file(GLOB PROJECT_SRCS
        "include/*.h" "src/*.cpp"
        )

add_executable(${PROJECT_NAME}
        ${LIBRARIES_PATH}/glad/debug/src/glad.c
        ${LIBRARIES_PATH}/objl
        ${IMGUI_SOURCES}
        ${ENGINE_SRCS}
       ${PROJECT_SRCS})

target_link_libraries(${PROJECT_NAME}
        debug zlibstaticd
        optimized zlibstatic)

target_link_libraries(
        ${PROJECT_NAME}
        debug engined
        optimized engine
)

target_link_libraries(
        ${PROJECT_NAME}
        debug reactphysics3dd
        optimized reactphysics3d
)

target_link_libraries(
        ${PROJECT_NAME} glfw3 opengl32 Ws2_32
)
