#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
CONFIG="${CONFIG:-RelWithDebInfo}"

cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${CONFIG}"
cmake --build "${BUILD_DIR}" --parallel
