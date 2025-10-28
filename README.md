# LiveStream Pull Player (Qt)

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B)](main.cpp) [![Qt 5](https://img.shields.io/badge/Qt-5-41CD52?logo=qt)](CMakeLists.txt) [![CMake](https://img.shields.io/badge/CMake-%E2%89%A53.20-064F8C?logo=cmake)](CMakeLists.txt)

一个简洁的桌面端直播拉流播放器示例，使用 Qt + CMake + 现代 C++ 实现，包含播放核心、渲染窗口与基础统计信息模块，便于二次开发与集成。当前项目基于 Qt 5。

---

## 功能特性

- 直播拉流播放与基础控制（播放/停止）
- 轻量渲染组件与窗口集成
- 基础播放统计（帧率/队列深度等）
- CMake 跨平台构建（Windows / macOS / Linux）
- 代码结构清晰，易于扩展

## 目录结构

```text
.
├── CMakeLists.txt
├── main.cpp
├── mainwindow.h
├── mainwindow.cpp
├── livestreamplayer.h
├── livestreamplayer.cpp
├── videowidget.h
├── videowidget.cpp
├── packetqueue.h
├── packetqueue.cpp
├── playerstats.h
└── .gitignore
```

关键文件：

- [CMakeLists.txt](CMakeLists.txt)：项目与依赖配置
- [main.cpp](main.cpp)：程序入口
- [mainwindow.h](mainwindow.h) / [mainwindow.cpp](mainwindow.cpp)：主窗口与 UI 逻辑
- [livestreamplayer.h](livestreamplayer.h) / [livestreamplayer.cpp](livestreamplayer.cpp)：播放核心
- [videowidget.h](videowidget.h) / [videowidget.cpp](videowidget.cpp)：视频渲染
- [packetqueue.h](packetqueue.h) / [packetqueue.cpp](packetqueue.cpp)：数据包队列
- [playerstats.h](playerstats.h)：播放统计

## 技术栈

- C++17
- Qt 5（Widgets 等）
- CMake ≥ 3.15

> 提示：如果使用了第三方编解码库（例如 FFmpeg），请在 [CMakeLists.txt](CMakeLists.txt) 中按需补充依赖查找与链接；Qt5 下通常使用 `find_package(Qt5 COMPONENTS Widgets REQUIRED)` 并链接 `Qt5::Widgets` 等目标。

## 环境准备

- 安装 Qt 5（建议 5.12.x；记录安装路径）
- 安装 CMake（3.15 及以上）
- 安装 C/C++ 编译器
  - Windows：MSVC 或 MinGW + Ninja
  - macOS：Xcode Command Line Tools
  - Linux：GCC/Clang，推荐同时安装 ninja-build

## 快速开始（构建与运行）

下述示例使用 CMake out-of-source 构建。请根据你本机 Qt 5 安装路径设置 CMAKE_PREFIX_PATH（若使用系统包管理安装的 Qt，通常可省略）。

### Windows（MSVC + Ninja）

```powershell
$env:CMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64"
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Windows（MinGW）

```powershell
$env:CMAKE_PREFIX_PATH="C:\Qt\5.15.2\mingw81_64"
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### macOS

```bash
export CMAKE_PREFIX_PATH="$HOME/Qt/5.15.2/clang_64"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Linux（Debian/Ubuntu 示例）

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build qtbase5-dev
# 如使用官方安装器安装 Qt 5：
export CMAKE_PREFIX_PATH="$HOME/Qt/5.15.2/gcc_64"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### 运行

- Windows：.\build\Release\ 或 .\build\ 目录下生成可执行文件
- macOS/Linux：./build/ 目录下生成可执行文件

> 如需将依赖一并打包发布，可参考 `windeployqt`、`macdeployqt` 或 Linux 发行打包工具。

## 使用说明

- 启动程序后，按界面指引输入或选择流地址并开始播放
- 播放时可在状态栏/调试输出查看基础统计信息
- 若需要自定义渲染或解码流程，可在 [livestreamplayer.cpp](livestreamplayer.cpp) 与 [videowidget.cpp](videowidget.cpp) 中扩展

## 配置与扩展

- 在 [CMakeLists.txt](CMakeLists.txt) 中添加或移除模块/库
- 若集成第三方 SDK，建议以 `FindXXX.cmake` 或 `FetchContent` 管理依赖
- 为便于调试，建议开启符号信息与日志宏开关

## 故障排查

- 构建时报找不到 Qt：检查并正确设置 `CMAKE_PREFIX_PATH`
- 运行时缺少 Qt 动态库：使用 `windeployqt`/`macdeployqt` 或修正运行路径
- 无法播放或花屏：检查输入流、编解码器可用性与线程队列状态（见 [packetqueue.*](packetqueue.cpp)）

## 规划路线

- [ ] 首次启动向导与预设配置
- [ ] 更多流协议与容错策略
- [ ] 播放统计可视化与导出
- [ ] CI 构建与自动化测试

## 贡献

欢迎提交 Issue 与 PR。提交前请：

- 遵循现有代码风格（C++17/Qt 规范）
- 提供最小可复现示例或单元测试（如适用）

## 许可证

当前未设置许可证。如需开源分发，建议添加常见许可证（MIT/Apache-2.0/BSD-3-Clause 等）并在 [CMakeLists.txt](CMakeLists.txt) 中补充相关元信息。

## 致谢

- [Qt](https://www.qt.io/)
- 开源社区与相关工具作者

---

如果本项目对你有帮助，欢迎 Star/Fork，或在企业内作为模板项目复用。
