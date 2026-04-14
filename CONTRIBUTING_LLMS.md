# Contributing With LLMs

This file defines guardrails for AI-assisted changes in this repo.
Goal: keep changes easy to review, deterministic, and architecture-aligned.

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

Avoid:
- generic registries without real need
- hidden callback chaining
- editor-only state as source of truth
- DSL indirection for core behavior

## Expected Validation

At minimum, run one desktop build:

```bash
cmake --build build-nophysics --target tiny_hippie_engine
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

## Commit Style

Use focused commit messages:
- `refactor scene node render component ownership`
- `fix web imgui callback forwarding`
- `add physics sandbox node debug names`

Avoid umbrella commits that mix unrelated refactors and behavior changes.

