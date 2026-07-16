# Repository Guidelines

## Project Structure & Module Organization

This repository builds the `yuri-music` C++23 desktop application. `src/main.cpp` is the
entry point. C++ module units live under `src/`: `pages/` contains screen-level views,
`components/` reusable UI widgets, `models/` application data, `views/` window composition,
and `tools/` platform integrations. Each area has an aggregator module such as
`pages/pages.cppm` or `components/components.cppm`. Shared bindings are in `libs/`, while
`libs/qq-music-api` is a Git submodule. SVG icons belong in `resources/svg/`; CMake copies
the complete `resources/` tree beside the executable. Platform dependency setup belongs in
`cmake/`.

## Build, Test, and Development Commands

Initialize dependencies before the first build:

```bash
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build build
./build/yuri-music
```

CMake 4.0+, Ninja, and a compiler with C++23 module support are required (Clang 19+ on
Linux; MSVC 19.38+ or GCC 15+ where supported). The build also expects the external
`yuri_lib` target and 64-bit BASS libraries described by `cmake/bass.cmake`; Windows builds
add WebView2. Use `cmake --build build --clean-first` after module dependency changes.

No automated tests or coverage threshold are currently configured. At minimum, contributors
must complete a clean build and manually exercise the affected page, playback control, or
login flow. If tests are added through CTest, run them with
`ctest --test-dir build --output-on-failure` and name test files after the feature under test.

## Coding Style & Naming Conventions

Before editing code, read `docs/style.md`; it is the authoritative project style
guide. Also run `clang-format -i <changed-file>` before committing. `.clang-format` specifies
two-space indentation, a 100-column limit, no tabs, right-aligned pointers, and ungrouped
parameters.

Keep files and module partitions in `snake_case` (`song_item.cppm`,
`components:song_item`). Use `PascalCase` for classes, `camelCase` for functions,
`snake_case` for variables and data members, and `kPascalCase` for constants. Preserve the
project's Chinese comments and user-facing copy where appropriate.

## Commit & Pull Request Guidelines

Recent history follows Conventional Commit prefixes, often with a scope, for example
`feat(yuri-music): 新增用户资料卡` or `fix(yuri-music): 修正歌曲列表布局`. Use a concise
imperative summary and keep unrelated changes separate. Pull requests should explain the
behavioral change, list build/manual verification, link relevant issues, and include
screenshots or a short recording for visible UI changes. Call out submodule updates and any
new runtime assets or platform dependencies explicitly.
