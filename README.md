# 09_LiveStreamPullPlayer

<div align="center">

**RTSP/RTMP 直播拉流播放器**

一个基于 Qt 和 FFmpeg 的高性能、低延迟网络直播流播放器

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Qt](https://img.shields.io/badge/Qt-5.12.9-green.svg)](https://www.qt.io/)
[![FFmpeg](https://img.shields.io/badge/FFmpeg-GPL-red.svg)](https://ffmpeg.org/)
[![CMake](https://img.shields.io/badge/CMake-3.15+-blue.svg)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

</div>

---

## 📝 项目简介

本项目是一个功能完备的网络直播流播放器,专注于提供稳定、低延迟的 RTSP/RTMP 流播放体验。通过精心设计的多线程架构、抖动缓冲机制和智能重连策略,能够在复杂网络环境下保持流畅播放。

### 核心特性

- 🎬 **多协议支持**: 支持 RTSP 和 RTMP 网络流协议
- ⚡ **低延迟播放**: 端到端延迟控制在 1-2 秒以内
- 🔄 **智能重连**: 网络中断时自动重连,可配置重试次数和间隔
- 📊 **实时统计**: 显示码率、队列状态、丢帧数等关键性能指标
- 🎨 **现代化 UI**: 采用扁平化设计,支持 SVG 矢量图标
- 🛡️ **稳定可靠**: 多线程架构配合抖动缓冲,确保播放流畅

---

## 🏗️ 技术架构

### 技术栈

| 组件 | 版本/说明 | 用途 |
|------|----------|------|
| **C++** | C++17 | 核心开发语言 |
| **Qt** | 5.12.9 | UI 框架和多媒体支持 |
| **FFmpeg** | GPL 版本 | 音视频解封装、解码 |
| **CMake** | 3.15+ | 跨平台构建系统 |
| **Qt Multimedia** | 5.12.9 | 音频输出 |

### 核心架构

项目采用**多线程生产者-消费者**模式,通过数据包队列实现线程间解耦:

```
┌─────────────┐      ┌──────────────┐      ┌───────────┐
│ 解封装线程   │ ───> │ 数据包队列    │ ───> │ 解码线程  │
│ (Demux)    │      │ (Jitter      │      │ (Decode) │
│            │      │  Buffer)     │      │          │
└─────────────┘      └──────────────┘      └───────────┘
      │                                           │
      │ 网络 I/O                                  │ 音视频输出
      ▼                                           ▼
┌─────────────┐                          ┌───────────┐
│ FFmpeg      │                          │ Qt Widget │
│ Network     │                          │ QAudio    │
└─────────────┘                          └───────────┘
```

#### 关键模块

1. **解封装线程 (Demux Thread)**
   - 负责网络 I/O 和流解封装
   - 实现超时控制和错误处理
   - 支持自动重连机制

2. **数据包队列 (PacketQueue)**
   - 作为抖动缓冲区吸收网络抖动
   - 支持两种溢出策略: 阻塞 (Block) 和丢弃最旧包 (DropOldest)
   - 音频队列采用阻塞策略保证连续性
   - 视频队列采用丢包策略保证低延迟

3. **解码线程 (Decode Threads)**
   - 独立的音频和视频解码线程
   - 使用 Qt 隐式共享机制减少内存拷贝
   - 音频通过定时器批量写入避免递归调用

4. **音视频同步**
   - 针对直播流优化的同步策略
   - 能够处理 PTS 不连续和回退情况

---

## 🚀 快速开始

### 环境要求

- **操作系统**: Windows 10/11 (支持跨平台扩展)
- **编译器**: MSVC 2019+ 或 MinGW GCC 8.0+
- **CMake**: 3.15 或更高版本
- **Qt**: 5.12.9 或更高版本
- **FFmpeg**: GPL 版本,包含所需库

### 安装步骤

#### 1. 安装依赖

**安装 Qt**:

```bash
# 从 Qt 官网下载安装器
# https://www.qt.io/download-open-source
```

**准备 FFmpeg**:

```bash
# 下载 FFmpeg GPL 版本
# 将 FFmpeg 解压到指定目录,例如: F:/Software/cpp_packages/ffmpeg-gpl
```

#### 2. 克隆项目

```bash
git clone https://github.com/huminglong/09_LiveStreamPullPlayer.git
cd 09_LiveStreamPullPlayer
```

#### 3. 配置和编译

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目 (需要设置 FFmpeg 路径)
cmake .. -DFFMPEG_ROOT="F:/Software/cpp_packages/ffmpeg-gpl"

# 编译项目
cmake --build . --config Debug
```

#### 4. 运行

```bash
# Windows
.\Debug\09_LiveStreamPullPlayer.exe
```

---

## 📖 使用说明

### 基本操作

1. **输入流地址**: 在 "📡 Stream URL" 输入框中输入 RTSP 或 RTMP 地址

   ```
   示例 RTSP: rtsp://192.168.1.100/stream1
   示例 RTMP: rtmp://live.example.com/live/stream
   ```

2. **配置重连参数**:
   - **🔄 最大重试**: 设置网络中断时的最大重连尝试次数 (默认: 5)
   - **⏱️ 重试间隔**: 设置每次重连尝试之间的等待时间(秒) (默认: 3)

3. **开始播放**: 点击 "▶️ 开始播放" 按钮

4. **停止播放**: 点击 "⏹️ 停止播放" 按钮

### 实时统计信息

播放过程中,界面底部会实时显示:

- **📹 Video Queue**: 视频数据包队列大小
- **🔊 Audio Queue**: 音频数据包队列大小
- **📊 Bitrate**: 当前流的码率 (MB/s)
- **⚠️ Dropped**: 由于队列满而丢弃的视频帧数量

---

## 📁 项目结构

```
09_LiveStreamPullPlayer/
├── CMakeLists.txt              # CMake 构建配置
├── main.cpp                    # 应用程序入口
├── mainwindow.h/.cpp          # 主窗口类 (UI 控制)
├── livestreamplayer.h/.cpp    # 核心播放器类 (多线程控制)
├── packetqueue.h/.cpp         # 数据包队列 (抖动缓冲)
├── playerstats.h              # 统计信息结构体
├── videowidget.h/.cpp         # 视频渲染组件
├── resources/                 # 资源文件
│   ├── resources.qrc          # Qt 资源配置
│   └── icons/                 # SVG 矢量图标
├── doc/                       # 文档目录
│   ├── 09_LiveStreamPullPlayer_PRD.md      # 产品需求文档
│   ├── OPTIMIZATION_SUMMARY.md              # 优化总结
│   ├── UI_OPTIMIZATION_SUMMARY.md           # UI 优化总结
│   └── ICONS_GUIDE.md                       # 图标使用指南
├── build/                     # 构建输出目录 (gitignored)
└── build_qt/                  # Qt Creator 构建目录 (gitignored)
```

### 核心文件说明

| 文件 | 职责 | 关键功能 |
|------|------|----------|
| `livestreamplayer.h/.cpp` | 播放器核心逻辑 | 多线程管理、FFmpeg 封装、重连逻辑 |
| `packetqueue.h/.cpp` | 抖动缓冲队列 | 线程安全队列、溢出策略、丢帧统计 |
| `mainwindow.h/.cpp` | 用户界面 | UI 布局、信号槽连接、状态显示 |
| `videowidget.h/.cpp` | 视频渲染 | QPainter 绘制、圆角裁剪、抗锯齿 |

---

## 🎨 界面优化

### 现代化设计元素

- **扁平化风格**: 简洁清爽的界面设计
- **渐变按钮**: 绿色播放按钮和红色停止按钮
- **圆角组件**: 所有输入框和按钮都采用圆角设计 (6px)
- **矢量图标**: 使用 SVG 图标,支持任意缩放不失真
- **状态反馈**: 清晰的连接状态和统计信息显示

### 配色方案

| 元素 | 颜色 | 用途 |
|------|------|------|
| 主背景 | `#f5f5f5` | 浅灰色背景 |
| 播放按钮 | `#4CAF50` | 绿色渐变 |
| 停止按钮 | `#f44336` | 红色渐变 |
| 输入框焦点 | `#4CAF50` | 绿色边框 |
| 文本主色 | `#333333` | 深灰色 |

---

## ⚡ 性能优化

### 关键优化技术

#### 1. 异步停止机制

- 使用 `std::shared_future` 实现非阻塞停止
- 避免 UI 线程冻结,提升用户体验

#### 2. 零拷贝视频帧传递

- 利用 Qt `QImage` 的隐式共享 (Copy-on-Write)
- 减少 50% 的内存拷贝操作

#### 3. 智能丢帧策略

- 视频队列满时丢弃最旧的包而非阻塞
- 音频队列采用阻塞策略保证连续性
- 实时统计丢帧数量

#### 4. 批量音频写入

- 使用队列 + 定时器替代递归调用
- 20ms 定时器批量处理音频数据
- 消除 UI 线程事件队列压力

#### 5. 实时统计刷新

- 400ms 定时器主动推送统计信息
- 相比原 1 秒更新提升 60% 响应速度

### 性能指标

| 指标 | 目标值 | 实际表现 |
|------|--------|----------|
| 端到端延迟 | < 2s | 1-2s |
| UI 响应时间 | < 100ms | 即时响应 |
| 内存占用 | 稳定 | 长时间运行无泄漏 |
| CPU 占用 | 中等 | 单核心 30-50% |

---

## 🔧 开发指南

### 编码规范

- **命名约定**:
  - 类名: PascalCase (如 `LiveStreamPlayer`)
  - 成员变量: m_ 前缀 + camelCase (如 `m_videoQueue`)
  - 函数名: camelCase (如 `updateStats`)
  - 常量: k 前缀 + PascalCase (如 `kQueueMaxPackets`)

- **注释规范**:
  - 使用 Doxygen 风格注释
  - 关键逻辑和难以理解的部分添加中文注释
  - 公共 API 必须有完整的函数注释

- **代码组织**:
  - 避免深层嵌套,使用早返回
  - 单个函数不超过 50 行 (复杂逻辑除外)
  - 相关功能组织在一起

### 构建配置

#### Debug 配置

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DFFMPEG_ROOT="YOUR_FFMPEG_PATH"
cmake --build . --config Debug
```

#### Release 配置

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DFFMPEG_ROOT="YOUR_FFMPEG_PATH"
cmake --build . --config Release
```

### 添加新功能

1. **添加新图标**:
   - 在 `resources/icons/` 创建 SVG 文件
   - 在 `resources/resources.qrc` 中注册
   - 重新编译项目

2. **扩展统计信息**:
   - 在 `playerstats.h` 添加新字段
   - 在 `livestreamplayer.cpp` 的 `updateStats()` 填充数据
   - 在 `mainwindow.cpp` 更新 UI 显示

3. **支持新协议**:
   - FFmpeg 原生支持的协议无需修改代码
   - 只需在 URL 输入框输入对应协议地址

---

## 🧪 测试建议

### 功能测试

1. **基本播放测试**:

   ```bash
   # 使用公开测试流
   rtsp://176.139.87.16/axis-mediamedia.amp
   ```

2. **网络中断测试**:
   - 播放过程中断开网络
   - 验证自动重连功能
   - 检查 UI 是否保持响应

3. **高码率测试**:
   - 播放 4K/8K 高清流
   - 观察丢帧统计
   - 监控内存和 CPU 占用

4. **长时间稳定性测试**:
   - 连续播放 24 小时
   - 检查内存是否泄漏
   - 验证音视频同步

### 性能测试

- **延迟测试**: 使用秒表对比原始流和播放画面
- **资源占用**: 使用任务管理器监控 CPU 和内存
- **丢帧分析**: 观察统计信息中的 Dropped 数值

---

## 🛣️ 开发路线

### 已完成 ✅

- [x] 基础 RTSP/RTMP 播放功能
- [x] 多线程架构和抖动缓冲
- [x] 自动重连机制
- [x] 实时统计信息
- [x] 现代化 UI 设计
- [x] 性能优化 (异步停止、零拷贝、智能丢帧)

### 计划中 🚧

- [ ] 硬件解码支持 (NVDEC/DXVA2/VideoToolbox)
- [ ] 零拷贝渲染 (OpenGL/Vulkan 纹理)
- [ ] 自适应队列大小
- [ ] 录制功能 (保存流到本地文件)
- [ ] 快照功能 (截取当前帧)
- [ ] 播放列表管理
- [ ] 多流同时预览
- [ ] WebRTC 协议支持

### 未来愿景 🌟

- [ ] 跨平台支持 (Linux, macOS)
- [ ] 移动端适配 (Android, iOS)
- [ ] 云端流媒体服务集成
- [ ] AI 增强功能 (画质增强、目标检测)

---

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request!

### 贡献流程

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 提交 Pull Request

### 代码审查标准

- 遵循项目编码规范
- 添加必要的注释
- 通过所有测试用例
- 更新相关文档

---

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

## 📧 联系方式

- **项目主页**: [GitHub - 09_LiveStreamPullPlayer](https://github.com/huminglong/09_LiveStreamPullPlayer)
- **问题反馈**: [GitHub Issues](https://github.com/huminglong/09_LiveStreamPullPlayer/issues)

---

## 🙏 致谢

- [FFmpeg](https://ffmpeg.org/) - 强大的多媒体处理框架
- [Qt](https://www.qt.io/) - 优秀的跨平台应用框架
- 所有贡献者和用户的支持

---

<div align="center">

**如果这个项目对您有帮助,请给我们一个 ⭐️**

Made with ❤️ by [huminglong](https://github.com/huminglong)

</div>
