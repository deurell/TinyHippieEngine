# Tiny Hippie Engine

A small C++20 OpenGL/WebGL2 starter engine for code-driven games and visual
experiments. The repository is trimmed to one starter scene,
`SkeletalAnimationBlendScene`, with a glTF character setup, animation blending,
simple flock behavior, debug UI, audio plumbing, and the reusable engine pieces
needed to grow a new project.

## Features

- Native desktop builds for macOS/Linux/Windows.
- WebGL2 build path via Emscripten.
- Scene graph based runtime with `SceneNode` transforms and render components.
- glTF/GLB mesh loading with animation clips, animation playback, and skinning helpers.
- Mesh rendering through `IRenderDevice` and the OpenGL backend.
- ImGui debug UI, runtime logs, scene tree, and inspector.
- Minimal starter resources: two character GLBs, their `Resources/Textures/`
  textures, and one mesh shader pair.
- Optional audio system and optional physics wrapper.

## Requirements

- CMake 3.15 or newer.
- A C++20 compiler.
- Ninja or Make.
- Emscripten SDK for web builds.

## Building

### Native Desktop

```bash
scripts/build_desktop.sh
CONFIG=Debug scripts/build_desktop.sh
```

### Tests

```bash
scripts/run_tests.sh
```

Physics is off by default. To include the optional ReactPhysics3D wrapper in
tests, run:

```bash
TINY_ENGINE_ENABLE_PHYSICS=ON scripts/run_tests.sh
```

### WebAssembly

```bash
export EMS="$HOME/emsdk"
scripts/build_web.sh
```

`EMS` may point to the emsdk root or directly to `upstream/emscripten`.

## Running

```bash
./build/tiny_hippie_engine
```

Controls:

- `WASD`: move the camera.
- Hold right mouse button and move the mouse: rotate the camera heading.
- `ESC`: quit.

## Project Layout

- `source/`, `include/`: engine and starter scene code.
- `Resources/`: starter character assets, with external GLB textures under
  `Resources/Textures/`.
- `Shaders/`: starter mesh shaders.
- `tests/`: focused runtime, animation, scene, and asset tests.
- `scripts/`: desktop, web, architecture, and test helpers.
- `imgui/`, `glfw/`, `transcoder/`: third-party code.
