#!/usr/bin/env bash
set -euo pipefail

paths=(
  include/renderdevice.h
  include/scenenode.h
  include/visualizerbase.h
  source/app.cpp
  source/debugui.h
  source/meshnode.cpp
  source/meshnode.h
  source/meshvisualizer.cpp
  source/meshvisualizer.h
  source/openglrenderdevice.cpp
  source/scenenode.cpp
  source/skeletalanimationblendscene.cpp
  source/skeletalanimationblendscene.h
)

scene_paths=()
for path in "${paths[@]}"; do
  if [[ "${path}" != "source/openglrenderdevice.cpp" &&
        "${path}" != "source/debugui.h" &&
        "${path}" != "source/app.cpp" ]]; then
    scene_paths+=("${path}")
  fi
done

for path in "${scene_paths[@]}"; do
  if [[ ! -f "${path}" ]]; then
    echo "Missing architecture check path: ${path}" >&2
    exit 1
  fi
done

forbidden_gl_pattern='\bgl[A-Z][A-Za-z0-9_]*\b|GL_[A-Z0-9_]+|<glad|\bGLuint\b|\bGLint\b|\bGLenum\b|\bGLsizei\b'
forbidden_window_pattern='GLFW|glfw|imgui_impl_opengl|imgui_impl_glfw'
forbidden_legacy_include_pattern='#include "(shader|texture|mesh|model|plane|textsprite|app)\.h"'

if rg -n "${forbidden_gl_pattern}" "${scene_paths[@]}"; then
  echo "Architecture check failed: OpenGL symbols leaked into clean scene paths." >&2
  exit 1
fi

if rg -n "${forbidden_window_pattern}" "${scene_paths[@]}"; then
  echo "Architecture check failed: window/backend symbols leaked into clean scene paths." >&2
  exit 1
fi

if rg -n "${forbidden_legacy_include_pattern}" "${scene_paths[@]}"; then
  echo "Architecture check failed: legacy rendering helpers leaked into clean scene paths." >&2
  exit 1
fi

if rg -n "createAlphaTexture" include source tests; then
  echo "Architecture check failed: createAlphaTexture should stay removed." >&2
  exit 1
fi
