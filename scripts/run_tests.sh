#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
TINY_ENGINE_ENABLE_PHYSICS="${TINY_ENGINE_ENABLE_PHYSICS:-OFF}"

scripts/check_architecture.sh
cmake -S . -B "${BUILD_DIR}" -DBUILD_TESTING=ON \
  -DTINY_ENGINE_ENABLE_PHYSICS="${TINY_ENGINE_ENABLE_PHYSICS}"
cmake --build "${BUILD_DIR}" --target tiny_hippie_engine_tests
ctest --test-dir "${BUILD_DIR}" --output-on-failure
