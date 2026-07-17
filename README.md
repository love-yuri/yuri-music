# yuri-music

一个使用 C++23 构建的桌面音乐客户端，界面基于
[Pixel Journey](https://github.com/love-yuri/pixel-journey)，通过
[qq-music-api](https://github.com/love-yuri/qq-music-api) 访问 QQ 音乐数据，并使用 BASS
提供音频播放能力。

## 功能

- 首页、浏览、搜索、音乐库、收藏和最近播放页面
- QQ 音乐账号登录与数据访问
- 歌曲播放控制
- Windows 与 Linux 桌面支持

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
