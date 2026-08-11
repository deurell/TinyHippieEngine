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
- Default actions are movement on `WASD` and `Fire` on `Space`.
- `InputState::moveAxis` is the preferred directional movement input; web
  virtual stick input writes it directly.
- Fixed timestep: `1/60` in `App`.
- Default window size: `1280x720`.
- `App` owns an `AudioSystem` (miniaudio backend); access via `app.audioSystem()`.
- Animation is `AnimationClip` + `AnimationPlayer` + `Skinning` — used with glTF-loaded models.
- Starter app registers the generic sample `TextStarterScene` first, then
  `InputDebugScene`, then the Tiny Dungeon atlas `TextStarterScene`, then the
  Kenney GLB `TextStarterScene`, then `SkeletalAnimationBlendScene`;
  physics-enabled builds also register `PhysicsTestScene`.
- Build targets separate reusable runtime (`tiny_hippie_runtime`), samples
  (`tiny_hippie_samples`), Deflektorish (`deflektorish_game`), and the application shell
  (`tiny_hippie_app`). `tiny_hippie_engine` injects the starter bootstrap and
  `deflektorish` injects the game bootstrap; these are separate executable
  composition roots, not runtime-selectable modes.
- Starter scene composition can be authored in text via `SceneDescription`
  JSON files, currently validating `SceneNode`, `CameraNode`, `MeshNode`,
  `LightNode`, `Light2DNode`, `SpriteNode`, `SpriteAnimationNode`,
  `SpriteBatchNode`, `FogOverlayNode`, `TextNode`, `TileMapNode`,
  `PlaneNode`, `ShaderPlaneNode`, `PhongShapeNode`, and
  `ParticleSystemNode`; `CameraNode` supports authored perspective or
  orthographic projection, `LightNode` provides one forward directional light
  for lit render components, `Light2DNode` provides additive 2D glows, `SpriteNode`
  supports atlas `sourceRect`, `flipX`, `flipY`, and `flipDiagonal`,
  `SpriteAnimationNode` advances atlas frames in `fixedUpdate()`,
  `SpriteBatchNode` batches many static atlas sprites from one image, and
  `FogOverlayNode` renders scrolling transparent mist/cloud overlays, and
  `ShaderPlaneNode` renders authored shader-driven quads with explicit blend,
  depth, and vec4 parameter buckets.
- Scene JSON authoring is validated with `scripts/tiny_hippie_validate.py`;
  it checks known fields, node types, required node payloads, enum values,
  tilemap/atlas invariants, and referenced asset paths.
- Starter runtime resources are `Resources/Scenes/simple_starter.scene.json`,
  `Resources/Scenes/tiny_dungeon_atlas.scene.json`,
  `Resources/Scenes/kenney_platformer.scene.json`,
  `Resources/Scenes/SCHEMA.md`, `Resources/C64_Pro-STYLE.ttf`,
  `character-l.glb`, `character-q.glb`, `Resources/Textures/texture-l.png`,
  `Resources/Textures/texture-q.png`,
  `Resources/Textures/generated/fog-soft-noise.png`,
  `Resources/Textures/generated/retro-crystal-terminal.png`,
  `Resources/Kenney/TinyDungeon/`,
  `Resources/Kenney/PlatformerKit/`,
  `Shaders/meshnode.*`,
  `Shaders/colored_line.*`, `Shaders/status.*`, `Shaders/image.*`,
  `Shaders/tilemap.*`, `Shaders/fogoverlay.*`, `Shaders/light2d.frag`,
  `Shaders/bloom_colorgrade.frag`,
  `Shaders/particle.vert`, `Shaders/particlefx.frag`,
  `Shaders/postprocess.vert`, `Shaders/chromatic_aberration.frag`, and
  `Shaders/crt.frag`.
- Rendering uses engine-owned passes: `Opaque`, `Overlay`, then a fullscreen `PostProcess` stack.
- Physics is optional (`TINY_ENGINE_ENABLE_PHYSICS`, default OFF); gated via `#ifdef TINY_ENGINE_ENABLE_PHYSICS`.
- Available typed node type in the starter is `MeshNode`.
- Available render component type in the starter is `MeshRenderComponent`.
- Raw OpenGL (`gl*`, `GL_*`, `GLFW`, `glfw`) must not appear in scene/node/render-component files — enforced by `scripts/check_architecture.sh`.

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
It builds `deflektorish` by default; set `TARGET=tiny_hippie_engine` for the
sample app or `TARGET=all` for both web executables.

Tiled map conversion:

```bash
scripts/convert_tiled_map.py input.tmx Resources/Scenes/output.scene.json --use-packed
```

Scene validation:

```bash
scripts/tiny_hippie_validate.py Resources/Scenes/*.scene.json
```

Build flags (CMake options):
- `TINY_ENGINE_ENABLE_IMGUI` (default ON) — debug UI; disables `USE_IMGUI` define when OFF.
- `TINY_ENGINE_ENABLE_PHYSICS` (default OFF) — Box3D wrapper sources; keep OFF unless the project needs physics.

Tests:

```bash
scripts/run_tests.sh
```

`run_tests.sh` runs `scripts/check_architecture.sh` first (requires `rg`),
validates scene JSON, then builds and runs CTest.

## Minimum Verification Before Merge

```bash
cmake --build build-nophysics --target tiny_hippie_engine
```

If web integration changed, run web build too and report result.
