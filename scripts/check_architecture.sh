#!/usr/bin/env bash
set -euo pipefail

paths=(
  include/renderdevice.h
  include/scenenode.h
  include/rendercomponent.h
  source/app.cpp
  source/debugui.h
  source/meshnode.cpp
  source/meshnode.h
  source/meshrendercomponent.cpp
  source/meshrendercomponent.h
  source/openglrenderdevice.cpp
  source/scenenode.cpp
  source/game/scenes/skeletalanimationblendscene.cpp
  source/game/scenes/skeletalanimationblendscene.h
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

app_boundary_paths=(include/app.h include/appbootstrap.h source/app.cpp)
if rg -n '#include "game/|Deflektorish|StarterBootstrap' "${app_boundary_paths[@]}"; then
  echo "Architecture check failed: application shell depends on game/sample content." >&2
  exit 1
fi

runtime_target=$(sed -n '/add_library(tiny_hippie_runtime STATIC/,/^)/p;
  /target_sources(tiny_hippie_runtime /,/^)/p' CMakeLists.txt)
if rg -n 'source/game/' <<<"${runtime_target}"; then
  echo "Architecture check failed: tiny_hippie_runtime contains game sources." >&2
  exit 1
fi

sample_paths=()
while IFS= read -r path; do
  sample_paths+=("${path}")
done < <(rg --files source/game | rg 'starterbootstrap|inputdebugscene|textstarterscene|skeletalanimationblendscene|physicstestscene')
if rg -n 'Deflektorish|game/deflektorish|deflektorishscene' "${sample_paths[@]}"; then
  echo "Architecture check failed: samples depend on Deflektorish." >&2
  exit 1
fi

while IFS= read -r header; do
  if [[ ! -f "include/${header}" ]]; then
    echo "Architecture check failed: public API smoke test includes private header '${header}'." >&2
    exit 1
  fi
done < <(sed -n 's/^#include "\([^"]*\)"/\1/p' tests/public_api_smoke.cpp)

public_api_headers=(
  appbootstrap.h
  audiosystem.h
  camera.h
  iscene.h
  lighting.h
  rendercomponent.h
  renderdevice.h
  renderpass.h
  scenemanager.h
  scenenode.h
)
for header in "${public_api_headers[@]}"; do
  while IFS= read -r dependency; do
    if [[ -f "source/${dependency}" && ! -f "include/${dependency}" ]]; then
      echo "Architecture check failed: public header '${header}' includes private header '${dependency}'." >&2
      exit 1
    fi
  done < <(sed -n 's/^#include "\([^"]*\)"/\1/p' "include/${header}")
done

if sed -n '/target_include_directories(tiny_hippie_runtime/,/^)/p' CMakeLists.txt |
   sed -n '/PUBLIC/,/PRIVATE/p' | rg -q '\$\{CMAKE_SOURCE_DIR\}/source'; then
  echo "Architecture check failed: tiny_hippie_runtime exports source/ as a public include directory." >&2
  exit 1
fi

if ! rg -q 'tiny_hippie_engine PRIVATE tiny_hippie_app' CMakeLists.txt ||
   ! rg -q 'deflektorish PRIVATE tiny_hippie_app' CMakeLists.txt; then
  echo "Architecture check failed: executable composition roots changed unexpectedly." >&2
  exit 1
fi

if rg -n -- '--preload-file Resources@|--preload-file Resources ' CMakeLists.txt; then
  echo "Architecture check failed: web targets must not package the entire Resources tree." >&2
  exit 1
fi
