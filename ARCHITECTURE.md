# Tiny Hippie Engine Architecture

This document captures the current runtime architecture and hard invariants.
If behavior in code diverges from this file, update this file in the same change.

## Scope

Tiny Hippie Engine is a code-first, cross-platform rendering/simulation engine.
It is intentionally not editor-first and not DSL-first.

Primary targets:
- Desktop OpenGL via GLFW/GLAD
- WebAssembly/WebGL2 via Emscripten

## Runtime Model

Core pieces:
- `App`: owns window, main loop, scene lifecycle, debug UI frame boundaries.
- `IScene`: scene contract (`init`, `fixedUpdate`, `update`, `render`, input hooks).
- `SceneNode`: scene-graph base class with local/world transforms + hierarchy.
- Render components (`VisualizerBase` descendants): attached to `SceneNode` and rendered from node world transforms.
- `IRenderDevice`: renderer abstraction with OpenGL implementation.

Starter content:
- The app registers one project scene: `SkeletalAnimationBlendScene`.
- Runtime resources are intentionally minimal: `character-l.glb`, `character-q.glb`,
  their PNG textures in `Resources/Textures/`, and `Shaders/meshnode.*`.
- `MeshNode` + `MeshVisualizer` are the active node/render component pair.

Current scene representation:
- Hierarchy and transforms are node-based (`SceneNode` tree).
- Rendering behavior is component-based (`addRenderComponent(...)` on nodes).
- Scene tree/debug selection is node-only. Components are listed in inspector metadata.

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
children (SceneNode)          renderComponents (VisualizerBase...)
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

Build flags:
- `TINY_ENGINE_ENABLE_IMGUI` (default ON) enables debug UI.
- `TINY_ENGINE_ENABLE_PHYSICS` (default OFF) builds the optional
  ReactPhysics3D wrapper sources.

Web:
- Use `scripts/build_web.sh`.
- `EMS` may point to either:
  - emsdk root (contains `upstream/emscripten/...`)
  - direct emscripten dir (`.../upstream/emscripten`)
- Script resolves toolchain file and validates `ninja` availability.

## Architecture Boundaries

What this repo intentionally is:
- Scene graph with explicit code-driven scenes and components.
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
