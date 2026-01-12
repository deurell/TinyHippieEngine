#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${EMS:-}" ]]; then
  echo "EMS environment variable must point to your Emscripten SDK directory." >&2
  exit 1
fi

WEB_DIR="${WEB_DIR:-web}"
CONFIG="${CONFIG:-MinSizeRel}"

cmake -S . -B "${WEB_DIR}" -G Ninja -DCMAKE_TOOLCHAIN_FILE="${EMS}/emscripten.cmake" -DCMAKE_BUILD_TYPE="${CONFIG}"
cmake --build "${WEB_DIR}"
