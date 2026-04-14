#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${EMS:-}" ]]; then
  echo "EMS environment variable must point to your Emscripten SDK directory." >&2
  exit 1
fi

WEB_DIR="${WEB_DIR:-web}"
CONFIG="${CONFIG:-MinSizeRel}"
TOOLCHAIN_FILE=""

if [[ -f "${EMS}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" ]]; then
  TOOLCHAIN_FILE="${EMS}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
elif [[ -f "${EMS}/cmake/Modules/Platform/Emscripten.cmake" ]]; then
  TOOLCHAIN_FILE="${EMS}/cmake/Modules/Platform/Emscripten.cmake"
fi

if [[ -z "${TOOLCHAIN_FILE}" ]]; then
  echo "Could not locate Emscripten CMake toolchain from EMS='${EMS}'." >&2
  echo "Expected one of:" >&2
  echo "  ${EMS}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" >&2
  echo "  ${EMS}/cmake/Modules/Platform/Emscripten.cmake" >&2
  exit 1
fi

if ! command -v ninja >/dev/null 2>&1; then
  echo "Ninja is required but was not found in PATH." >&2
  exit 1
fi

cmake -S . -B "${WEB_DIR}" -G Ninja -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" -DCMAKE_BUILD_TYPE="${CONFIG}"
cmake --build "${WEB_DIR}"
