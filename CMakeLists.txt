cmake_minimum_required(VERSION 3.26)
project(ShooterGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Engine (https://github.com/kewldan/Engine): a sibling checkout by default,
# otherwise fetched from GitHub.
# ---------------------------------------------------------------------------
set(ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Engine" CACHE PATH "Path to a checkout of kewldan/Engine")

if (EXISTS "${ENGINE_DIR}/CMakeLists.txt")
    message(STATUS "Using Engine from ${ENGINE_DIR}")
    add_subdirectory("${ENGINE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/Engine")
else ()
    message(STATUS "ENGINE_DIR (${ENGINE_DIR}) not found, fetching Engine from GitHub")
    include(FetchContent)
    FetchContent_Declare(Engine
            GIT_REPOSITORY https://github.com/kewldan/Engine.git
            GIT_TAG main)
    FetchContent_MakeAvailable(Engine)
endif ()

# ---------------------------------------------------------------------------
# Third-party (vcpkg manifest, see vcpkg.json)
# ---------------------------------------------------------------------------
find_package(OpenGL REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(plog CONFIG REQUIRED)
find_package(Stb REQUIRED)
# miniaudio is a single header (see Audio.cpp for the implementation unit).
find_path(MINIAUDIO_INCLUDE_DIR miniaudio.h REQUIRED)

add_executable(${PROJECT_NAME}
        src/Audio.cpp
        src/Chat.cpp
        src/Effects.cpp
        src/Frustum.cpp
        src/GameObject.cpp
        src/GBuffer.cpp
        src/Lights.cpp
        src/main.cpp
        src/Mesh.cpp
        src/Minimap.cpp
        src/Player.cpp
        src/PostProcess.cpp
        src/ScreenQuad.cpp
        src/ShadowsCaster.cpp
        src/Skybox.cpp
        src/SSAO.cpp
        src/Sun.cpp
        src/Weapon.cpp
        src/World.cpp
        # vendored: https://github.com/hnOsmium0001/imgui-command-palette (MIT)
        thirdparty/imcmd/imcmd_command_palette.cpp
        thirdparty/imcmd/imcmd_fuzzy_search.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
        include
        thirdparty/imcmd
        thirdparty/objloader   # vendored: https://github.com/Bly7/OBJ-Loader (MIT)
        ${BULLET_INCLUDE_DIRS}
        ${Stb_INCLUDE_DIR}
        ${MINIAUDIO_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Engine
        ${BULLET_LIBRARIES}
        imgui::imgui
        glad::glad
        plog::plog
        OpenGL::GL
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /utf-8)
    # Chat.cpp uses the portable, bounded strncpy on purpose; silence MSVC's "use strncpy_s" warning.
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif ()

# ---------------------------------------------------------------------------
# Data files.
# Debug builds read data/ from the working directory. Release builds (NDEBUG) make the Engine read
# shaders and textures from RCDATA resources keyed by their relative path, so generate a .rc that
# embeds them. Meshes (data/meshes) are always read from disk.
# ---------------------------------------------------------------------------
if (WIN32)
    enable_language(RC)
    file(GLOB_RECURSE _resource_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS
            "data/shaders/*" "data/textures/*")
    set(_rc_content "")
    foreach (_file IN LISTS _resource_files)
        string(APPEND _rc_content "${_file} RCDATA \"${CMAKE_CURRENT_SOURCE_DIR}/${_file}\"\n")
        list(APPEND _rc_deps "${CMAKE_CURRENT_SOURCE_DIR}/${_file}")
    endforeach ()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/resources.rc.in" "${_rc_content}")
    configure_file("${CMAKE_CURRENT_BINARY_DIR}/resources.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/resources.rc" COPYONLY)
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")
    # The .rc only names the files, so the resource compiler is not re-run when a shader or texture
    # changes; tell the build system about the real dependencies.
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/resources.rc" PROPERTIES OBJECT_DEPENDS "${_rc_deps}")
endif ()

# Keep data/ next to the executable so it can be started from the build directory too.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/data"
        COMMENT "Copying data/ next to ${PROJECT_NAME}")

set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
