# reVC 自定义功能说明

本文档记录了为 reVC 添加的三个自定义功能。

## 功能 1: macOS 系统信息显示

### 实现位置
- `src/core/re3.cpp` - PrintMacOSSystemInfo() 函数
- `src/core/Game.cpp` - InitialiseOnceBeforeRW() 函数调用

### 功能描述
当游戏在 macOS 上启动时，会在终端打印详细的系统信息，包括：
- 操作系统名称和版本
- 内核版本
- 机器类型（arm64/x86_64）
- CPU 型号和核心数
- CPU 频率
- 物理内存大小
- 设备型号

### 输出示例
```
==========================================
我正在运行于macOS上！
==========================================
系统信息:
  操作系统: Darwin
  系统版本: 24.0.0
  内核版本: Darwin Kernel Version 24.0.0
  机器类型: arm64
  主机名称: YourMac

CPU信息:
  CPU型号: Apple M3 Max
  CPU核心数: 16
  CPU频率: 3.50 GHz

内存信息:
  物理内存: 128.00 GB

硬件信息:
  设备型号: Mac15,6
==========================================
```

### 技术实现
使用了以下 macOS 系统 API：
- `uname()` - 获取系统基本信息
- `sysctlbyname()` - 获取 CPU 和内存详细信息
  - `machdep.cpu.brand_string` - CPU 型号
  - `hw.ncpu` - CPU 核心数
  - `hw.cpufrequency` - CPU 频率
  - `hw.memsize` - 物理内存
  - `hw.model` - 设备型号

---

## 功能 2: 帧数限制选项

### 实现位置
- `src/core/Frontend.h` - 添加 m_PrefsFPSLimit 变量
- `src/core/MenuScreensCustom.cpp` - 添加菜单选项和回调函数
- `src/core/re3.cpp` - 添加配置读取和保存逻辑

### 功能描述
在游戏菜单的 **设置 → 显示设置** 中添加了 "FPS Limit" 选项，提供三档帧率限制：
- 30 FPS
- 60 FPS（默认）
- 120 FPS

### 使用方法
1. 启动游戏
2. 进入主菜单
3. 选择 **OPTIONS（选项）** → **DISPLAY SETTINGS（显示设置）**
4. 找到 **FPS Limit** 选项
5. 使用左右方向键切换帧率限制
6. 设置会自动保存到 `reVC.ini` 文件中

### 技术实现
- 使用 `CCFOSelect` 创建下拉选择菜单
- 通过 `FPSLimitChanged` 回调函数实时更新 `RsGlobal.maxFPS`
- 配置保存在 `reVC.ini` 的 `[Display]` 部分，键名为 `FPSLimit`

### 配置文件示例
```ini
[Display]
FPSLimit=1
```
值对应关系：
- 0 = 30 FPS
- 1 = 60 FPS
- 2 = 120 FPS

---

## 功能 3: 脚本执行日志

### 实现位置
- `src/control/Script.cpp` - ProcessOneCommand() 函数

### 功能描述
在游戏运行时，实时打印所有任务脚本的执行信息到终端，格式如下：
```
[script] 执行命令 ID=<命令ID>, IP=<指令指针>, 脚本名=<脚本名称>
```

### 输出示例
```
[script] 执行命令 ID=0, IP=1234, 脚本名=main
[script] 执行命令 ID=1, IP=1236, 脚本名=main
[script] 执行命令 ID=4, IP=1240, 脚本名=main
[script] 执行命令 ID=78, IP=1245, 脚本名=mission1
[script] 执行命令 ID=150, IP=1250, 脚本名=mission1
```

### 功能说明
- **命令 ID**: GTA 脚本命令的唯一标识符
- **IP (Instruction Pointer)**: 脚本当前执行位置
- **脚本名**: 正在执行的脚本文件名（例如任务脚本）

### 注意事项
- 只有标记为 `m_bMissionFlag` 的任务脚本才会被打印
- 主脚本（main script）的常规命令不会被打印，避免输出过多
- 可以通过重定向输出到文件来保存日志：
  ```bash
  ./reVC 2>&1 | tee script.log
  ```

### 调试用途
此功能主要用于：
- 调试任务脚本逻辑
- 分析脚本执行流程
- 定位任务卡顿或错误问题
- 学习 GTA 脚本系统

---

## 编译说明

### 编译 Debug 版本（包含所有功能）
```bash
cd /Users/prelinakaren/Documents/reVC/build
make -j5 config=debug_macosx-arm64-librw_gl3_glfw-oal
```

### 编译 Release 版本
```bash
cd /Users/prelinakaren/Documents/reVC/build
make -j5 config=release_macosx-arm64-librw_gl3_glfw-oal
```

### 编译产物位置
- Debug: `/Users/prelinakaren/Documents/reVC/bin/macosx-arm64-librw_gl3_glfw-oal/Debug/reVC.app`
- Release: `/Users/prelinakaren/Documents/reVC/bin/macosx-arm64-librw_gl3_glfw-oal/Release/reVC.app`

---

## 测试方法

### 测试功能 1（macOS 系统信息）
1. 从终端启动游戏：
   ```bash
   cd '/Users/prelinakaren/Downloads/Grand.Theft.Auto.Vice.City.CHS.Green/Grand Theft Auto Vice City'
   ./reVC
   ```
2. 查看终端输出，应该能看到系统信息

### 测试功能 2（帧数限制）
1. 启动游戏
2. 进入 OPTIONS → DISPLAY SETTINGS
3. 找到 "FPS Limit" 选项并切换
4. 使用 Fraps 或系统监视器观察帧率变化

### 测试功能 3（脚本日志）
1. 从终端启动游戏
2. 开始一个任务
3. 观察终端输出的 `[script]` 日志行

---

## 源代码修改清单

### 新增文件
无

### 修改的文件
1. `src/core/re3.cpp`
   - 添加 `PrintMacOSSystemInfo()` 函数
   - 添加 FPS 限制配置读取/保存
   - 添加 macOS 头文件引用

2. `src/core/Game.cpp`
   - 在 `InitialiseOnceBeforeRW()` 中调用 `PrintMacOSSystemInfo()`

3. `src/core/Frontend.h`
   - 添加 `m_PrefsFPSLimit` 成员变量

4. `src/core/MenuScreensCustom.cpp`
   - 添加 `fpsLimitOptions` 数组
   - 添加 `FPSLimitChanged()` 回调函数
   - 在 DISPLAY_SETTINGS 菜单添加 FPS Limit 选项

5. `src/control/Script.cpp`
   - 在 `ProcessOneCommand()` 中添加脚本打印逻辑

---

## 兼容性说明

- **平台**: macOS (arm64/x86_64)
- **最低系统**: macOS 10.15+
- **构建系统**: Premake5 + GNU Make
- **依赖库**: OpenAL-soft, GLFW, mpg123, libsndfile

所有功能都是可选的，不影响游戏核心功能。如果某个功能出现问题，可以：
- 功能 1: 在非 macOS 平台自动禁用
- 功能 2: 默认使用 60 FPS
- 功能 3: 不影响游戏运行，只影响日志输出

---

## 后续改进建议

### 功能 1
- 添加 GPU 信息获取
- 添加显示器分辨率信息
- 将信息保存到日志文件

### 功能 2
- 添加自定义 FPS 输入框
- 添加动态 FPS 调整（根据负载）
- 添加 FPS 计数器显示

### 功能 3
- 添加可配置的日志级别
- 添加日志文件输出选项
- 添加命令名称解析（而不只是 ID）
- 添加参数值打印

---

## 作者信息

**开发者**: GitHub Copilot  
**日期**: 2025年11月3日  
**版本**: 1.0.0  
**基于项目**: reVC (Reverse-engineered GTA Vice City)

## 许可证

本自定义功能遵循 reVC 项目的原始许可证。
