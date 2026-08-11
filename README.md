# Tiny Hippie Engine

A small C++20 OpenGL/WebGL2 starter engine for code-driven games and visual
experiments. The default starter scene is a `TextStarterScene` loading a Kenney
Tiny Dungeon atlas sample. Additional scenes include a Kenney Platformer Kit GLB
sample, the generic node sample scene, and `SkeletalAnimationBlendScene` as a
richer glTF animation and flocking example.

## Features

- Native desktop builds for macOS/Linux/Windows.
- WebGL2 build path via Emscripten.
- Scene graph based runtime with `SceneNode` transforms and render components.
- Text-authored scene composition for LLM/coder-friendly node setup.
- Atlas-backed `TileMapNode` support for compact 2D tile-map scenes.
- glTF/GLB mesh loading with animation clips, animation playback, and skinning helpers.
- Mesh rendering through `IRenderDevice` and the OpenGL backend.
- ImGui debug UI, runtime logs, scene tree, and inspector.
- Minimal starter resources: JSON scene descriptions, atlas sprites, GLB meshes,
  texture assets, and shader pairs.
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

The desktop build produces two applications:

- `tiny_hippie_engine`: generic starter and sample scenes.
- `deflektorish`: the Deflektorish game.

### Tests

```bash
scripts/run_tests.sh
```

Physics is off by default. To include the optional Box3D wrapper in tests, run:

```bash
TINY_ENGINE_ENABLE_PHYSICS=ON scripts/run_tests.sh
```

### WebAssembly

```bash
export EMS="$HOME/emsdk"
scripts/build_web.sh
```

`EMS` may point to the emsdk root or directly to `upstream/emscripten`.

### Tiled Maps

Tiled `.tmx` maps can be converted into JSON `TileMapNode` scenes:

```bash
scripts/convert_tiled_map.py input.tmx Resources/Scenes/output.scene.json --use-packed
```

## Running

```bash
./build/tiny_hippie_engine
# or
./build/deflektorish
```

Controls:

- `WASD`: move the camera.
- Hold right mouse button and move the mouse: rotate the camera heading.
- `ESC`: quit.

## Project Layout

- `source/`, `include/`: engine and starter scene code.
- `tiny_hippie_runtime`: reusable engine library target with no game sources.
- `tiny_hippie_samples`: engine demonstrations used by the starter app.
- `deflektorish_game`: game-only behavior and content integration.
- `tiny_hippie_app`: platform application loop shared by both executables.
- `AppBootstrap`: narrow application composition hook; each executable injects
  its own scene registration and application-specific state without adding
  mode branches to the shared loop.
- `Resources/`: starter character and Kenney sample assets, with external GLB
  textures under `Resources/Textures/` and text scene descriptions plus schema
  notes under `Resources/Scenes/`.
- `Shaders/`: starter mesh shaders.
- `tests/`: focused runtime, animation, scene, and asset tests.
- `scripts/`: desktop, web, architecture, and test helpers.
- `imgui/`, `glfw/`, `transcoder/`: third-party code.
