#!/usr/bin/env bash

CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" || exit 1

if ! command -v cmake >/dev/null 2>&1; then
  printf '[ERROR] CMake 4.0 or newer is required in PATH.\n' >&2
  exit 1
fi
if ! command -v ninja >/dev/null 2>&1; then
  printf '[ERROR] Ninja is required in PATH.\n' >&2
  exit 1
fi
if ! command -v clang++ >/dev/null 2>&1; then
  printf '[ERROR] Clang 19 or newer is required in PATH.\n' >&2
  exit 1
fi

printf '[1/2] Configuring Release...\n'
if ! cmake -S . -B build/release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=clang++ \
  "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$PWD/output" \
  -DBUILD_TESTING=OFF -DBUILD_DEMO=OFF; then
  printf '[ERROR] Release configuration failed. See the messages above.\n' >&2
  exit 1
fi

printf '[2/2] Building Release...\n'
if ! cmake --build build/release --config Release --target yuri-music --parallel; then
  printf '[ERROR] Release build failed. See the messages above.\n' >&2
  exit 1
fi

printf '[OK] Release output: "%s"\n' "$PWD/output"
