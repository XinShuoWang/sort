#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=${1:-build}

ctest --test-dir "$BUILD_DIR/test" --output-on-failure || true

find "$BUILD_DIR" -name "*.gcda" -o -name "*.gcno" >/dev/null || {
  echo "No coverage data (.gcda/.gcno). Ensure ENABLE_COVERAGE=ON and rebuild.";
  exit 1;
}

gcov -pb $(find "$BUILD_DIR" -name "*.gcda") >/dev/null || true

echo "Coverage artifacts generated (gcov). For HTML, install lcov and run:
  lcov --capture --directory $BUILD_DIR --output-file coverage.info && \
  genhtml coverage.info --output-directory coverage-html"
