# Tiny Hippie Engine

A tiny cross-platform demo engine for experimenting with GL/ImGui scenes on macOS/Linux/Windows and WebAssembly. It bundles BasisU texture decoding, mini audio playback, and a scene manager for demo-style effects.

## Features
- Native builds for macOS/Linux/Windows plus WebGL2 via Emscripten.
- BasisU transcoding for compact texture assets (`Resources/*.basis`).
- A grab bag of demo scenes (Intro, NodeExample, DemoScene, ParticleScene, etc.) you can switch between at runtime (`←/→`).

## Requirements
- CMake ≥ 3.15, Ninja/Make, a C++20 compiler (Clang/GCC/MSVC).
- For Web builds you’ll need the Emscripten SDK; set `EMS` to the path containing `emscripten.cmake`.

## Building

### Native Desktop
```bash
scripts/build_desktop.sh                # default RelWithDebInfo into build/
CONFIG=Debug scripts/build_desktop.sh   # change build type
```

### WebAssembly (WebGL2)
```bash
export EMS="$HOME/emsdk/upstream/emscripten"
scripts/build_web.sh                    # emits web/tiny_hippie_engine.html
```
Serve `web/` via `python -m http.server` and open the HTML to run in a browser.

## Running
```bash
./build/tiny_hippie_engine
```

Controls:
- `←/→` cycle scenes.
- `SPACE` loads the simple shader scene.
- `ESC` quits.

Many scenes expose ImGui panels for tweaking parameters.

## Structure
- `include/`, `source/` – Engine headers/implementations.
- `Shaders/`, `Resources/` – GLSL and assets copied next to the binary.
- `scripts/` – Build helpers for desktop/web targets.
- `imgui/`, `glfw/`, `transcoder/` – third-party code.

Enjoy tinkering! <3
