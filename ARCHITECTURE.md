# Tiny Hippie Engine Architecture

This document captures the current runtime architecture and hard invariants.
If behavior in code diverges from this file, update this file in the same change.

## North Star

Tiny Hippie Engine is optimized for LLM-friendly, single-developer game
development. The engine should stay slim, explicit, and easy to inspect.

Principles:
- Keep the authored surface tiny and text-first. Prefer readable JSON, GLSL,
  PNG, TTF, GLB, TMX/TSX source assets, and generated text artifacts over
  custom binary formats.
- Keep runtime code boring and typed. JSON creates normal C++ nodes; it does not
  become a second gameplay language.
- Prefer validation over editor complexity. A fast command-line validator is
  more valuable than hidden editor behavior.
- Keep the blessed node set small. New node types should be reusable engine
  primitives, not one-off game behavior.
- Keep samples and tests as executable documentation. Every broadly reusable
  node feature should have schema docs and parser/validator coverage.
- Avoid hidden magic. Defaults, units, ownership, and render behavior should be
  visible in code or docs.

## Scope

Tiny Hippie Engine is a code-first, cross-platform rendering/simulation engine.
It supports text-authored composition for LLM/coder cooperation, but runtime
behavior remains typed C++.

Primary targets:
- Desktop OpenGL via GLFW/GLAD
- WebAssembly/WebGL2 via Emscripten

## Runtime Model

Core pieces:
- `App`: owns window, main loop, scene lifecycle, debug UI frame boundaries.
- `IScene`: scene contract (`init`, `fixedUpdate`, `update`, `render`, input hooks).
- `SceneNode`: scene-graph base class with local/world transforms + hierarchy.
- `CameraNode`: scene-graph camera node with authored transform/projection.
- Render components (`RenderComponent` descendants): attached to `SceneNode` and rendered from node world transforms.
- `IRenderDevice`: renderer abstraction with OpenGL implementation.
- `SceneDescription`: JSON-authored scene composition that builds normal runtime
  nodes through `SceneNodeFactory`.
- `scripts/tiny_hippie_validate.py`: command-line scene validator used for fast
  authoring feedback before running the engine.

Starter content:
- The app registers the generic sample `TextStarterScene` first, then the
  Tiny Dungeon atlas `TextStarterScene`, then the Kenney GLB `TextStarterScene`,
  then `SkeletalAnimationBlendScene`.
  Physics-enabled builds also register `PhysicsTestScene`.
- Runtime resources are intentionally minimal:
  `Resources/Scenes/simple_starter.scene.json`,
  `Resources/Scenes/tiny_dungeon_atlas.scene.json`,
  `Resources/Scenes/kenney_platformer.scene.json`, `character-l.glb`,
  `Resources/Scenes/SCHEMA.md`, `Resources/C64_Pro-STYLE.ttf`,
  `character-q.glb`, their PNG textures in `Resources/Textures/`,
  `Resources/Textures/generated/fog-soft-noise.png`,
  `Resources/Textures/generated/retro-crystal-terminal.png`,
  `Resources/Kenney/TinyDungeon/`,
  `Resources/Kenney/PlatformerKit/`,
  `Shaders/meshnode.*`, `Shaders/colored_line.*`, `Shaders/status.*`,
  `Shaders/image.*`, `Shaders/tilemap.*`, `Shaders/fogoverlay.*`,
  `Shaders/light2d.frag`, `Shaders/particle.vert`,
  `Shaders/bloom_colorgrade.frag`, `Shaders/particlefx.frag`,
  `Shaders/postprocess.vert`,
  `Shaders/chromatic_aberration.frag`, and `Shaders/crt.frag`.
- `MeshNode` + `MeshRenderComponent` are the active node/render component pair.

Build-time ownership boundaries:
- `tiny_hippie_runtime` is the reusable engine library. It contains no
  `source/game` files and owns the scene graph, rendering, assets, animation,
  audio, persistence, and optional physics implementation.
- `tiny_hippie_samples` owns reusable sample scenes and the starter bootstrap.
- `deflektorish_game` owns Deflektorish gameplay, content integration, and its
  bootstrap. Game features move into `tiny_hippie_runtime` only after they are
  proven reusable, then receive a focused sample in `tiny_hippie_samples`.
- `tiny_hippie_app` owns the GLFW application loop and debug UI integration. It
  receives an `AppBootstrap` and has no starter or Deflektorish branches.
- `tiny_hippie_engine` injects `StarterBootstrap`; `deflektorish` injects
  `DeflektorishBootstrap`. The executable is the composition root: there is no
  runtime mode toggle between the two applications.
- Bootstraps may register scenes and own application-specific state through the
  narrow configure/resources/update/before-render lifecycle. They do not
  override the platform loop or simulation timing.
- Tests link the runtime and game-content libraries instead of recompiling
  engine implementation sources.

Current scene representation:
- Hierarchy and transforms are node-based (`SceneNode` tree).
- Rendering behavior is component-based (`addRenderComponent(...)` on nodes).
- World units are meters: `1.0` scene unit represents roughly one meter for
  authored transforms, sample spacing, camera movement, and future physics.
- Text scene files may describe composition, transforms, mesh paths, render
  settings, and animation defaults. C++ scenes bind to named/typed nodes for
  behavior.
- The default scene node factory supports `SceneNode`, `CameraNode`,
  `LightNode`, `Light2DNode`, `MeshNode`, `SpriteNode`,
  `SpriteAnimationNode`, `SpriteBatchNode`, `FogOverlayNode`, `TextNode`,
  `TileMapNode`, `PlaneNode`, `PhongShapeNode`, and `ParticleSystemNode`.
- `CameraNode` supports perspective and orthographic projection modes from
  scene JSON. Perspective uses `fov`; orthographic uses `orthographicHeight`
  as vertical world-space view size.
- `LightNode` provides one scene directional light for forward-lit render
  components (`MeshNode` and `PhongShapeNode`). Unlit render components ignore it.
- `Light2DNode` provides authored additive radial glow overlays for 2D scenes.
- `SpriteNode` supports full-image sprites and atlas regions through
  `sourceRect`, `flipX`, `flipY`, and `flipDiagonal` in scene JSON.
- `SpriteAnimationNode` supports fixed-step atlas-frame animation through
  `spriteAnimation.frames`, `fps`, `playing`, and `looping` in scene JSON.
- `SpriteBatchNode` supports many static atlas-backed sprite quads from one
  image in one mesh/draw command.
- `FogOverlayNode` supports transparent scrolling texture overlays for mist,
  clouds, and cloud shadows.
- `TileMapNode` supports compact atlas-backed orthogonal maps with Tiled-style
  global tile IDs and flip flags.
- `scripts/convert_tiled_map.py` converts Tiled TMX/TSX content into
  JSON-authored `TileMapNode` scenes; runtime scene loading stays JSON-only.
- `scripts/tiny_hippie_validate.py` validates scene JSON structure, known node
  fields, node type payloads, enum values, common atlas/tilemap invariants, and
  referenced asset paths. It may warn about suspicious authored transforms, but
  warnings are advisory unless explicitly promoted by tooling.
- Scene tree/debug selection is node-only. Components are listed in inspector metadata.

Current render pass model:
- The app owns pass order.
- Scene-node render components are evaluated in `Opaque` then `Overlay`.
- `DrawCommand`s can opt into back-to-front sorting with a renderer-facing sort
  depth. Text, sprite, and particle render components use this for alpha/additive
  content so JSON node order does not decide whether later planes overdraw
  earlier translucent samples.
- The starter runtime renders scene content into an offscreen color+depth target,
  then executes a fullscreen `PostProcess` effect stack before debug UI.
- Postprocess effects are stack entries defined by shader paths + uniforms, and
  run through ping-pong render targets so effects are composable without
  hard-coding each effect into the frame loop.

## Architecture Diagram

```text
Desktop/Web Frame Driver
          |
          v
        App
  (window + loop + input + scene lifecycle)
          |
          +-----------------------------+
          |                             |
          v                             v
   fixedUpdate()                    update()/render()
 (0..N times per frame)            (1 time per frame)
          |                             |
          v                             |
      Scene/IScene ---------------------+
          |
          v
      SceneNode (root)
   (T*R*S local, parent->child world)
          |
    +-----+------------------------------+
    |                                    |
    v                                    v
children (SceneNode)          renderComponents (RenderComponent...)
 hierarchy + transforms         draw using node world transform
```

```text
Input path (authoritative):
GLFW callbacks owned by App
  -> ImGui_ImplGlfw_*Callback(...)
  -> App input handlers (scene controls/game input)
```

```text
Scene lifecycle and switching:

App::registerScenes()
    -> SceneManager (stores the starter scene factory)

App::loadCurrentScene()
    -> SceneManager::createCurrent()
    -> replacePreparedScene(previous, next, windowSize, framebufferSize)
         -> prepareScene(next, windowSize, framebufferSize)
              -> scene->init()
              -> scene->onScreenSizeChanged(...)
              -> scene->onFramebufferSizeChanged(...)
```

## Update And Timing

Main loop (desktop):
- `update()`
- `render()`

Main loop (web):
- `emscripten_set_main_loop_arg(...)` drives `update()` + `render()` via browser frame scheduling.

Fixed-step invariant:
- Fixed timestep is `1/60` seconds.
- `fixedUpdate()` runs zero or more times per render frame to consume accumulated time.
- `update()` runs once per render frame and is variable-rate.

Rules:
- Gameplay/simulation state belongs in `fixedUpdate()`.
- Presentation, smoothing, and UI belong in `update()`/`render()`.
- Do not make simulation correctness depend on render FPS.

## Input Flow

GLFW callbacks are owned by the app.
ImGui receives explicit forwarded events from app callbacks.

Invariant:
- `ImGui_ImplGlfw_InitForOpenGL(window, false)` is used.
- App callbacks call `ImGui_ImplGlfw_*Callback(...)` first, then app input handlers.
- Default actions are `MoveForward`, `MoveBackward`, `MoveLeft`, `MoveRight`,
  and `Fire`, bound to `W`, `S`, `A`, `D`, and `Space`.
- `InputState::moveAxis` is the preferred directional input for movement:
  `x` is right, `y` is forward/up in the input plane. Keyboard fills it from
  `WASD`; the web virtual stick writes it directly.

This avoids brittle callback chaining and is the expected integration for this repo.

## Transform Math

`SceneNode` local transform order:
- `local = T * R * S`
- `world = parentWorld * local`

Consequences:
- Parent scale affects child translation in world space.
- Parent rotation affects child axes and translation direction.

Debug override:
- Nodes can enable transform override for inspector-driven edits.
- When enabled, normal scene writes via `setLocalPosition/Rotation/Scale` are ignored.
- Inspector uses debug setters to apply explicit authoring values.

## Debug UI

The debug UI is a runtime observability layer, not source-of-truth content authoring.

Current windows:
- `Engine`: frame/simulation/render stats + pause/step controls.
- `Scene Tree`: node hierarchy only.
- `Inspector`: selected node transform + world values + attached component list.
- `Logs`: logger controls and filterable output.

Principle:
- Keep UI thin and reflective of runtime state.
- Do not move core gameplay logic into debug UI.

## Build System Notes

Desktop:
- Use `scripts/build_desktop.sh`.

Validation:
- Use `scripts/tiny_hippie_validate.py Resources/Scenes/*.scene.json` to check
  authored scene files without launching the app.
- `scripts/run_tests.sh` runs the scene validator before configuring and
  executing CTest.

Build flags:
- `TINY_ENGINE_ENABLE_IMGUI` (default ON) enables debug UI.
- `TINY_ENGINE_ENABLE_PHYSICS` (default OFF) builds the optional Box3D wrapper
  sources.

Web:
- Use `scripts/build_web.sh`.
- `EMS` may point to either:
  - emsdk root (contains `upstream/emscripten/...`)
  - direct emscripten dir (`.../upstream/emscripten`)
- Script resolves toolchain file and validates `ninja` availability.

## Architecture Boundaries

What this repo intentionally is:
- Scene graph with explicit code-driven scenes and components.
- Text-authored scene composition that stays diffable and easy for humans/LLMs
  to edit.
- Small, inspectable abstractions.

What this repo intentionally is not:
- Full ECS runtime with generic system scheduling.
- Editor-centric asset graph pipeline.
- DSL-driven scene behavior.

## Evolution Guidelines

Prefer:
- explicit ownership and typed references
- local refactors that reduce indirection
- deterministic simulation behavior

Avoid:
- stringly-typed runtime routing
- hidden callback ownership
- abstractions justified only by hypothetical future use
