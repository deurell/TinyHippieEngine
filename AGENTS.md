# AGENTS

Repository collaboration guide for humans and coding agents.

For full context:
- Architecture: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- LLM contribution rules: [`CONTRIBUTING_LLMS.md`](CONTRIBUTING_LLMS.md)

## Current Ground Truth

- Engine model: scene graph (`SceneNode`) + attached render components.
- Scene tree tooling is node-only.
- Inspector edits node transforms and supports per-node transform override.
- Main stats panel is `Engine` (old frame-stats overlay is not used in app flow).
- Input integration: app-owned GLFW callbacks explicitly forward to ImGui callbacks.
- Fixed timestep: `1/60` in `App`.
- Default window size: `1280x720`.

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

## Minimum Verification Before Merge

```bash
cmake --build build-nophysics --target tiny_hippie_engine
```

If web integration changed, run web build too and report result.
