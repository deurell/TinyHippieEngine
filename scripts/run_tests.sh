#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"

scripts/check_architecture.sh
cmake -S . -B "${BUILD_DIR}" -DBUILD_TESTING=ON
cmake --build "${BUILD_DIR}" --target tiny_hippie_engine_tests
ctest --test-dir "${BUILD_DIR}" --output-on-failure
