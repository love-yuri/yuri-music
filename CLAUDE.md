# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Build Skia (first time only)
python script/build.py

# Configure and build
mkdir -p build && cd build
cmake .. -G Ninja
cmake --build .

# Run tests
ctest
```

- CMake >= 4.0 + Ninja required
- C++23 with modules, compiler: MSVC >= 19.38 / GCC >= 15.0 / Clang >= 19.0
- Windows: uses vcpkg (`E:/love-yuri/github/vcpkg` or `$VCPKG_ROOT`), static CRT
- Linux: forces clang++, links additional system libs (fontconfig, freetype, icu, harfbuzz, etc.)
- Skia is a prebuilt static lib from hardcoded paths (`D:/skia` on Windows, `/home/yuri/github/skia` on Linux)

## Architecture

This is a C++23 UI framework (`yuri_lib`) built on Vulkan + Skia + GLFW. The project language is Chinese (comments, commit messages, docs).

### Module Dependency Graph (bottom-up)

```
configuration (is_debug_mode, vulkan constants)
       │
  ┌────┼────┬────┐
  core vulkan glfw skia
  │    │    │    │
  └────┴────┴────┘
       │
      ui (widgets, layout, render, animation, algorithm)
```

- **core** — Signal/Slot, logging, profiling (FPS counter, frame clock), type traits, UUID
- **vulkan** — Wraps vulkan-hpp via module barrier pattern; global singleton `vulkan_context` manages device/queue/command pool; detail helpers for Instance/Swapchain creation
- **glfw** — Window creation + Vulkan surface + swapchain + per-frame resources (`render_frame`, `WindowContext`). `glfw::Window` inherits `ui::widgets::Widget` and runs the main loop (acquire → update → layout → render → present → poll)
- **skia** — Re-exports Skia types via module barrier; `create_surface()` wraps Vulkan images into SkSurface; `SkPaintBuilder` fluent API; font/color resources
- **ui** — The main framework:
  - **widgets/** — `Widget` base class: parent-child tree, mouse event hit-testing/propagation, layout dirty flags, animation integration via `startAnimation<T>()`. Components: `Box`, `Button`, `Text`, `Splitter`
  - **layout/** — `Layout<Widget>` template (friend of Widget, calls `setGeometry()`). HBox/VBox distribute equally. `Alignment` is bitmask-based. `LayoutDirty` propagates up the tree
  - **render/** — `RenderNode` base → `RenderBackground`, `RenderBorder`, `RenderText`, `RenderSvg`. Widget owns render nodes, calls them during `paint()`
  - **animation/** — `IAnimation` interface, `LinearAnimation<T>` / `BezierAnimation<T>` (requires `CanLerp` concept). Global `AnimationManager` singleton updates per frame
  - **algorithm/** — Generic `lerp<T>()` with `SkColor` specialization (HSV interpolation)

### Key Design Patterns

- **Module barrier**: Low-level C headers (vulkan-hpp, GLFW, Skia) included in global module fragment, re-exported as C++ modules
- **Singletons**: `vulkan_context`, `animation_manager`, `frame_clock` as global pointers
- **Composite tree**: `Widget` parent-child with recursive render and event dispatch
- **Signal/Slot**: Custom `Signal<Args...>` + `function_ref` for callbacks (e.g. `Button::clicked`)
- **Friend-based layout access**: `Layout<Widget>` is friend of `Widget` to set geometry directly

### Coding Conventions

- **声明与实现分离**：修改或新增的 class 必须分离声明与实现，除非类非常短（简单 POD 或只有几行的小工具类）
- **函数注释**：每个公开函数声明上方必须添加简短中文注释说明用途
- **命名风格**：新增命名（类名、函数名、变量名）须保持与同模块已有代码一致的风格
- **字段注释**：所有字段最好添加注释

### File Conventions

- `.cppm` — C++ module implementation units
- `.ixx` — C++ module interface units (used for vulkan/api, vulkan/detail, skia, glfw context, configuration)
- Each module subdirectory has an aggregator file (e.g. `ui.cppm`) that re-exports its sub-modules
- clang-format: LLVM-based, 2-space indent, 100 column limit, pointer right-aligned, `BinPackParameters: false`

### Test / Example Apps

- `test/vulkan_hello.cpp` — Minimal Vulkan random-color demo, built as CTest
- 仓库根目录 — **实际音乐应用项目**（QQ Music clone），基于 yuri_lib 构建，包含多个页面和 SVG 资源
- `test/validation-html/` — 验证用 HTML 文件，用于对照设计稿效果

### Project Roles

- `src/` — 通用 UI 框架（yuri_lib），服务于上层应用
- 仓库根目录 — 实际业务项目，最终产品
- `test/validation-html/` — 设计稿验证参考
