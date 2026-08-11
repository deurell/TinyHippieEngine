# Contributing With LLMs

This file defines guardrails for AI-assisted changes in this repo.
Goal: keep changes easy to review, deterministic, and architecture-aligned.
The engine goal is LLM-friendly, single-developer, slim/tiny game development:
small text-authored surfaces, typed C++ runtime behavior, and fast validation
instead of hidden editor magic.

## Primary Workflow

1. Read relevant code before proposing architecture changes.
2. Prefer direct code changes over speculative planning text.
3. Keep patches scoped and testable.
4. Build and report what was verified.
5. Keep docs synchronized when architecture contracts change.

## Hard Contracts

- Keep simulation correctness in `fixedUpdate()`.
- Keep variable-rate presentation in `update()`/`render()`.
- Do not introduce render-FPS-dependent gameplay state changes.
- Keep scene hierarchy node-based (`SceneNode`).
- Keep rendering attached via explicit render components on nodes.
- Keep scene tree/selection node-only.
- Do not reintroduce string-key component lookup where typed ownership is clearer.
- Keep app-owned GLFW callbacks with explicit ImGui forwarding.

## Design Direction

Prefer:
- simple, explicit ownership
- typed pointers/references
- small abstractions with current use
- code-first scene logic
- text-authored data with clear validation
- reusable engine primitives over game-specific nodes
- samples/tests/docs that can be copied by humans and LLMs

Avoid:
- generic registries without real need
- hidden callback chaining
- editor-only state as source of truth
- DSL indirection for core behavior
- custom binary authoring formats without a readable source artifact
- broad node catalogs where a small composable node would do

## Engine And Game Ownership

- Keep reusable primitives in `tiny_hippie_runtime`.
- Keep sample composition in `tiny_hippie_samples` and game rules/content in
  `deflektorish_game`.
- Prototype uncertain functionality in the game. Promote only the smallest
  proven reusable contract into the runtime, with runtime tests and a focused
  sample demonstrating it independently.
- Do not make the shared `App` branch on application identity. Each executable
  injects its own `AppBootstrap` composition root.
- Treat the headers exercised by the API smoke consumer as the public baseline.
  Its header closure must remain independent of `source/`, and the runtime must
  not export `source/` as a public include directory. Do not imply that every
  header in `include/` is already a standalone installed SDK.

## Adding Scene-Authored Features

When adding or changing scene JSON support:
- Update `SceneDescription` parsing/building.
- Update `Resources/Scenes/SCHEMA.md`.
- Update `scripts/tiny_hippie_validate.py` with fields, enum values, required
  payloads, and asset references.
- Add or update parser/validator tests or a validated sample scene.
- Keep generated assets referenced by at least one scene, test, or doc.

## Expected Validation

At minimum, run one desktop build:

```bash
cmake --build build-nophysics --target tiny_hippie_engine
```

When touching scene JSON, node parser fields, or authored assets, run:

```bash
scripts/tiny_hippie_validate.py Resources/Scenes/*.scene.json
```

When touching web build or Emscripten integration, also run:

```bash
EMS=/path/to/emsdk ./scripts/build_web.sh
```

If a check cannot be run, call it out explicitly in the change summary.

## Change Checklist

Before finalizing:
- Architecture invariants still hold.
- New APIs have clear ownership and naming.
- Debug UI changes do not own gameplay logic.
- Input routing remains deterministic and explicit.
- Relevant docs updated (`ARCHITECTURE.md`, `AGENTS.md`, README when needed).
- Scene validator updated when scene-authored fields change.

## Commit Style

Use focused commit messages:
- `refactor scene node render component ownership`
- `fix web imgui callback forwarding`
- `add physics sandbox node debug names`

Avoid umbrella commits that mix unrelated refactors and behavior changes.
