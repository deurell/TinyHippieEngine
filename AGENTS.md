# AGENTS

Repository collaboration guide for humans and coding agents.

For full context:
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- LLM contribution rules: [`CONTRIBUTING_LLMS.md`](CONTRIBUTING_LLMS.md)

Precedence:
- `ARCHITECTURE.md` is authoritative for runtime and design contracts.
- `AGENTS.md` is a concise operational summary.

## Current Ground Truth

- Engine model: scene graph (`SceneNode`) + attached render components.
- Scene tree tooling is node-only.
- Inspector edits node transforms and supports per-node transform override.
- Main stats panel is `Engine` (old frame-stats overlay is not used in app flow).
- Input integration: app-owned GLFW callbacks explicitly forward to ImGui callbacks.
- Fixed timestep: `1/60` in `App`.
- Default window size: `1280x720`.
- `App` owns an `AudioSystem` (miniaudio backend); access via `app.audioSystem()`.
- Animation is `AnimationClip` + `AnimationPlayer` + `Skinning` — used with glTF-loaded models.
- Starter app registers only `SkeletalAnimationBlendScene`.
- Starter runtime resources are `character-l.glb`, `character-q.glb`, `Resources/Textures/texture-l.png`, `Resources/Textures/texture-q.png`, and `Shaders/meshnode.*`.
- Physics is optional (`TINY_ENGINE_ENABLE_PHYSICS`, default OFF); gated via `#ifdef TINY_ENGINE_ENABLE_PHYSICS`.
- Available typed node type in the starter is `MeshNode`.
- Available visualizer type in the starter is `MeshVisualizer`.
- Raw OpenGL (`gl*`, `GL_*`, `GLFW`, `glfw`) must not appear in scene/node/visualizer files — enforced by `scripts/check_architecture.sh`.

## Working Agreements

- Keep gameplay/simulation correctness in `fixedUpdate()`.
- Keep rendering/UI/presentation in `update()` and `render()`.
- Prefer typed ownership over string-based lookup.
- Keep patches small and reviewable.
- Update docs when changing architecture contracts.

## Build Commands

Desktop:

```bash
scripts/build_desktop.sh
```

Web:

```bash
EMS=/path/to/emsdk ./scripts/build_web.sh
```

`build_web.sh` accepts `EMS` as either emsdk root or direct `upstream/emscripten` path.

Build flags (CMake options):
- `TINY_ENGINE_ENABLE_IMGUI` (default ON) — debug UI; disables `USE_IMGUI` define when OFF.
- `TINY_ENGINE_ENABLE_PHYSICS` (default OFF) — ReactPhysics3D wrapper sources; keep OFF unless the project needs physics.

Tests:

```bash
scripts/run_tests.sh
```

`run_tests.sh` runs `scripts/check_architecture.sh` first (requires `rg`), then builds and runs CTest.

## Minimum Verification Before Merge

```bash
cmake --build build-nophysics --target tiny_hippie_engine
```

If web integration changed, run web build too and report result.
