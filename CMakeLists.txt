cmake_minimum_required(VERSION 3.23)
project(ShooterGame)

add_definitions(-DNETWORKING_PORT=23403)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 20)
include("C:/Users/kewldan/Desktop/Engine/Import.cmake")

include_directories(
        include
)

file(GLOB PROJECT_SRCS
        "include/*.h" "src/*.cpp"
        )

add_executable(${PROJECT_NAME}
        ${ENGINE_SRCS}
       ${PROJECT_SRCS}
        ${VCPKG_LIBRARIES_PATH}/include/imcmd_command_palette.cpp
        ${VCPKG_LIBRARIES_PATH}/include/imcmd_fuzzy_search.cpp
        )

target_link_libraries(
        ${PROJECT_NAME}
        Bullet3Common${_DL}
        BulletInverseDynamics${_DL}
        BulletDynamics${_DL}
        BulletCollision${_DL}
        BulletSoftBody${_DL}
        LinearMath${_DL}
        glfw3
        opengl32
        glad
        tb64
        brotlicommon-static
        brotlidec-static
        brotlienc-static
        Ws2_32
        imgui${_D}
        freetype${_D}
        bz2${_D}
        libpng16${_D}
        zlib${_D}
)
