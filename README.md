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
- 🎨 **Deferred rendering** — a G-buffer geometry pass (sRGB albedo), a fullscreen sun/ambient lighting pass and warm point lights drawn as light volumes (frustum culled spheres shading the G-buffer)
- 🌈 **HDR pipeline** — the scene is lit in linear HDR (RGBA16F), then bloom (soft-knee threshold, 5-mip down/upsample chain), ACES / Uncharted 2 tone mapping with exposure and FXAA 3.11, all tweakable in the Graphics tree
- 🌞 **Sun & god rays** — a depth-tested sun sprite and screen-space radial light shafts (half resolution, 64 taps) that fade as the sun leaves the screen
- 🌑 **SSAO** — screen-space ambient occlusion with a blur pass and quality presets (None / Low / Medium / High)
- ☀️ **Cascaded shadow maps** — four 2048×2048 PCF-filtered cascades rendered from the sun, frustum culled per cascade, toggleable in-game
- 🌅 **Skybox** — cubemap sky rendered in its own pass
- 🧭 **Live minimap** — a top-down render-to-texture pass that rotates with the camera, shown in the settings window
- 🏃 **Physics-driven player** — Bullet capsule rigid body with walking, slow-walk, a raycast ground check (jump only when grounded, coyote time, reduced air control) and a teleport-reset; the map collides with its real triangles
- 🔫 **Shooting** — hitscan shots via Bullet raycasts with bullet-hole decals, tracers, a muzzle flash, a 17-round magazine with reloads and shootable physics crates
- 🔦 **View-model weapon** — the Glock drawn in its own forward pass with sway, walk bob and recoil
- 🔭 **Aim-down-sights** — right mouse button smoothly narrows the horizontal FOV from 90° to 45° and centres the sights
- 🔊 **Sound** — miniaudio playback of procedurally generated effects (`tools/gen_sounds.py`): shots, reloads, footsteps, jumps, landings, hits
- 🌐 **UDP multiplayer** — a listen server (`--host`) and clients (`--connect`) on a small custom protocol: client-authoritative movement, server-validated hits with health, kills and respawns, interpolated remote players with nameplates, host-driven crates, chat, ping and a loss/latency simulator (see [Multiplayer](#-multiplayer))
- 💬 **In-game chat & command palette** — an ImGui chat console (`?players` lists who is connected) and a fuzzy-search command palette (`imgui-command-palette`)
- 🛠️ **Debug UI** — FPS/position/facing overlay, ammo/health/K-D HUD, sensitivity/speed sliders, VSync toggle, culling and network counters; start options `--pos x y z --yaw deg --pitch deg --novsync --test-shots N --aim --tracer-life s --say TEXT --console` for reproducible views

## 🎮 Controls

| Input | Action |
|---|---|
| `W` `A` `S` `D` | Move |
| Mouse | Look around |
| `LMB` | Fire |
| `RMB` (hold) | Aim down sights (zoom) |
| `R` | Reload |
| `Shift` | Slow walk |
| `Space` | Jump |
| `Backspace` | Reset player to spawn |
| `Esc` | Toggle mouse capture |
| `Ctrl` + `Shift` + `P` | Command palette |

## 🌐 Multiplayer

One player hosts, the others connect; the host is an ordinary player whose process also runs the server. Without `--host`/`--connect` the game is single-player.

```powershell
ShooterGame.exe --host [23403] --name Alice          # listen server on UDP port 23403 (default)
ShooterGame.exe --connect 192.168.1.10[:23403] --name Bob
ShooterGame.exe --connect 127.0.0.1 --simulate-loss 0.3 --simulate-latency 150   # a bad link, for testing
```

Protocol (`include/net/Protocol.h`):

1. UDP datagrams of at most 1200 bytes: `magic u16 | version u8 | type u8 | seq u16 | ack u16 | ackBits u32`, then reliable events, then an optional unreliable payload; little-endian, every field bounds-checked, malformed packets are dropped.
2. Unreliable channel: each client sends its `PlayerState` (position, velocity, yaw/pitch, flags) at 30 Hz; the server relays a snapshot of everybody plus the host's crate transforms at 20 Hz.
3. Reliable channel: join/accept/leave/shot/hit/respawn/chat events carry ids, are re-sent every 100 ms until a packet containing them is acknowledged (33-packet ack window), and are delivered once, in order. Heartbeat 500 ms, timeout 5 s.
4. Movement is client-authoritative; hits are not: the shooter sends its ray, the server tests it against the other players' capsules and the host's map/crates, applies 25 damage per hit (100 hp, respawn after 3 s) and broadcasts the outcome, which becomes tracers, decals, sounds and the damage flash on every client.
5. Remote players are rendered 100 ms behind the newest snapshot (interpolated between two snapshots, extrapolated for up to 250 ms when one is late) as kinematic capsules with the player model, so they take part in shadows, the minimap and local raycasts.

Ping (round trip from the acks) and packet rates are shown under Settings → Debug → Network.

## 🛠️ Tech stack

| Dependency | Purpose |
|---|---|
| [glad](https://github.com/Dav1dde/glad) + [GLFW](https://www.glfw.org/) | OpenGL loading & windowing |
| [Bullet3](https://github.com/bulletphysics/bullet3) | Physics (dynamics world, rigid bodies) |
| [Dear ImGui](https://github.com/ocornut/imgui) + [imgui-command-palette](https://github.com/hnOsmium0001/imgui-command-palette) | UI, chat, command palette |
| Winsock 2 | UDP multiplayer (`include/net/`) |
| [stb](https://github.com/nothings/stb) | Image loading |
| [miniaudio](https://miniaud.io/) | Sound playback |
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
