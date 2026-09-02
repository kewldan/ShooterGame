# 🔫 ShooterGame

> A first-person shooter prototype on a de_dust2 map — deferred rendering, SSAO, shadow mapping and Bullet physics in modern OpenGL.

[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat&logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/20)
[![OpenGL](https://img.shields.io/badge/OpenGL-glad-5586A4?style=flat&logo=opengl&logoColor=white)](https://www.opengl.org/)
[![GLFW](https://img.shields.io/badge/GLFW-3-orange?style=flat)](https://www.glfw.org/)
[![Bullet](https://img.shields.io/badge/physics-Bullet3-8A2BE2?style=flat)](https://github.com/bulletphysics/bullet3)
[![Dear ImGui](https://img.shields.io/badge/Dear%20ImGui-UI-4b8bbe?style=flat)](https://github.com/ocornut/imgui)
[![Platform](https://img.shields.io/badge/platform-Windows-0078D6?style=flat&logo=windows&logoColor=white)](#)

A sandbox for graphics and gameplay experiments: walk around CS's de_dust2 as a physics-driven capsule, with a multi-pass renderer running shadows, SSAO and deferred lighting every frame.

## ✨ Features

- 🗺️ **de_dust2 map** — the classic map as an OBJ mesh with its original textures, plus weapon models (Glock 17, sniper rifle) and a player model
- 🎨 **Deferred rendering** — a G-buffer geometry pass followed by a lighting pass with light volumes
- 🌑 **SSAO** — screen-space ambient occlusion with a blur pass and quality presets (None / Low / Medium / High)
- ☀️ **Shadow mapping** — a 4096×4096 depth map rendered from the sun (marked WIP in-game)
- 🌅 **Skybox** — cubemap sky rendered in its own pass
- 🧭 **Live minimap** — a top-down render-to-texture pass that rotates with the camera, shown in the settings window
- 🏃 **Physics-driven player** — Bullet capsule rigid body with walking, slow-walk, jumping and a teleport-reset
- 🔭 **Aim-down-sights** — right mouse button smoothly narrows the FOV from 60° to 25°
- 💬 **In-game chat & command palette** — an ImGui chat console and a fuzzy-search command palette (`imgui-command-palette`)
- 🛠️ **Debug UI** — FPS/position/facing overlay, ammo & health HUD, sensitivity/speed sliders, VSync toggle

## 🎮 Controls

| Input | Action |
|---|---|
| `W` `A` `S` `D` | Move |
| Mouse | Look around |
| `RMB` (hold) | Aim down sights (zoom) |
| `Shift` | Slow walk |
| `Space` | Jump |
| `Backspace` | Reset player to spawn |
| `Esc` | Toggle mouse capture |
| `Ctrl` + `Shift` + `P` | Command palette |

## 🛠️ Tech stack

| Dependency | Purpose |
|---|---|
| [glad](https://github.com/Dav1dde/glad) + [GLFW](https://www.glfw.org/) | OpenGL loading & windowing |
| [Bullet3](https://github.com/bulletphysics/bullet3) | Physics (dynamics world, rigid bodies) |
| [Dear ImGui](https://github.com/ocornut/imgui) + [imgui-command-palette](https://github.com/hnOsmium0001/imgui-command-palette) | UI, chat, command palette |
| [stb](https://github.com/nothings/stb) | Image loading |
| FreeType, libpng, zlib, bz2, Brotli, Turbo-Base64 | Fonts, textures, compression |
| Author's in-house `Engine` | Window, input, shaders, camera, HUD |

## 🔨 Building

Requirements: **CMake ≥ 3.26**, a C++20 MSVC toolchain and [vcpkg](https://github.com/microsoft/vcpkg) (dependencies are declared in `vcpkg.json`).

The game links against the author's [Engine](https://github.com/kewldan/Engine). CMake looks for it in `../Engine` next to this repository (override with `-DENGINE_DIR=<path>`); if it is not there it is fetched from GitHub automatically.

```powershell
git clone https://github.com/kewldan/Engine.git
git clone https://github.com/kewldan/ShooterGame.git
cd ShooterGame

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_TOOLCHAIN_FILE=<vcpkg root>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

`data/` (meshes, shaders, textures) is copied next to the executable after each build, so `build/ShooterGame.exe` can be started directly. Release builds additionally embed the shaders and textures as Windows resources, which is how the Engine loads them when `NDEBUG` is defined.

The command palette ([imgui-command-palette](https://github.com/hnOsmium0001/imgui-command-palette)) and the OBJ loader ([OBJ-Loader](https://github.com/Bly7/OBJ-Loader)) are vendored under `thirdparty/`.
