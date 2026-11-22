# yuri-music

一个使用 C++23 构建的桌面音乐客户端，界面基于
[Pixel Journey](https://github.com/love-yuri/pixel-journey)，通过
[qq-music-api](https://github.com/love-yuri/qq-music-api) 访问 QQ 音乐数据，并使用 BASS
提供音频播放能力。

## 功能

- 首页、浏览、搜索、音乐库、收藏和最近播放页面
- QQ 音乐账号登录与数据访问
- 歌曲播放控制
- Windows 与 Linux Wayland 桌面支持
- Linux 托盘：关闭主窗口后继续播放，托盘菜单仅提供「退出」
- Waybar 歌词：在状态栏显示当前播放的 QQ 音乐歌词

## 项目依赖

| 项目 | 用途 |
| --- | --- |
| [Pixel Journey](https://github.com/love-yuri/pixel-journey) | 项目的 C++23 桌面 UI 框架，基于 Vulkan、Skia 和 GLFW |
| [qq-music-api](https://github.com/love-yuri/qq-music-api) | QQ 音乐 API 的 C++23 封装，负责登录、用户资料、歌单和歌曲数据访问 |
| [BASS](https://www.un4seen.com/bass.html) | 音频播放与 FLAC 支持 |


## 环境要求

- CMake 4.0+
- Ninja
- 支持 C++23 Modules 的编译器
  - Clang 19+
  - MSVC 19.38+
  - GCC 15+
- 网页登录窗口依赖
  - Windows：WebView2
  - Linux：WebKitGTK（Arch 安装 `webkit2gtk-4.1`，Debian/Ubuntu 安装
    `libwebkit2gtk-4.1-dev`）
- Linux 版需要原生 Wayland 会话。窗口后端的构建依赖见
  [Pixel Journey 构建文档](libs/pixel-journey/README.md#快速开始)。
- Linux 托盘使用 GIO（`gio-2.0`，由 GLib 提供）。

## 构建与运行

```bash
git clone --recursive git@github.com:love-yuri/yuri-music.git
cd yuri-music

cmake -S . -B build -G Ninja
cmake --build build
```

运行程序：

```bash
# Linux
./build/yuri-music

# Windows
./build/yuri-music.exe
```

一键 Release 编译（需先安装上述依赖）：

Linux（使用 `clang++`）：

```bash
./yuri-music.sh
```

Windows（需安装 Visual Studio C++ 桌面开发工具）：

```bat
yuri-music.bat
```

两个脚本都使用 `build/release` 作为独立构建目录，将可执行文件、运行时库、
`resources/` 和 `script/` 输出到项目根目录的 `output/`。
编译后进入 `output/`，Linux 运行 `./yuri-music`，Windows 运行 `yuri-music.exe`。
Windows 脚本会自动加载 x64 MSVC 环境。
Windows Release 程序启动时不显示控制台窗口，Debug 构建保留控制台输出。

### Waybar 托盘

在 Waybar 的 `modules-right`（或其它模块列表）中启用 `"tray"` 即可。
托盘注册成功后，关闭主窗口不会停止播放。左键双击 `yuri-music` 图标可重新打开主窗口，
右键菜单仅提供「退出」。未注册托盘时，关闭窗口会直接退出。

### Waybar 歌词

在 Waybar 的 `modules-center`（或其它模块列表）中加入 `"custom/yuri-lyrics"`，
并在配置顶层添加以下模块。将 `exec` 中的路径改成实际的绝对路径；
`yuri-music-waybar` 随应用一起构建，位于 `build/` 或一键编译的 `output/` 中。

```jsonc
"custom/yuri-lyrics": {
  "exec": "/绝对路径/yuri-music-waybar",
  "interval": 1,
  "format": "{text}",
  "max-length": 48,
  "escape": true,
  "hide-empty-text": true,
  "tooltip": false
}
```

重新加载 Waybar 后，播放有时间轴歌词的歌曲即可显示当前行。暂停时保留当前行，
拖动进度或切歌后会更新，窗口隐藏到托盘时也会继续同步。没有歌词、停止播放或应用退出时模块隐藏。
歌词由 QQ 音乐单独提供，即使使用本地音频缓存，获取歌词仍需要网络。

## 项目结构

```text
src/          应用源码与 C++ 模块
resources/    SVG 等运行时资源
libs/         项目绑定与 Git 子模块
cmake/        平台和依赖配置
docs/         项目文档
```

## 许可证

本项目使用 [MIT License](LICENSE)。
