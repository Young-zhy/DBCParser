# DBC 文件解析与 CAN 帧发送工具

## 一、项目简介

本项目基于 **C++20 + Qt（Widgets）** 实现，主要功能包括：

- 解析标准 DBC 文件（Message / Signal）
- 以树形结构展示 CAN 信号
- 支持用户输入 Signal 物理值（Physical Value）
- 根据 Factor / Offset 自动换算 Raw 值
- 按 DBC 位定义打包生成 CAN Data（支持 Intel / Motorola）
- 通过 **WebSocket** 发送数据
- 使用 **Protobuf**进行高效序列化

------

## 二、项目结构

```shell
.
|   README.md
|
+---DBCParser
|   |   CMakeLists.txt
|   |   README.md
|   |   vcpkg.json
|   |
|   \---src
|       |   dbc_parser.cpp
|       |   dbc_parser.h
|       |   main.cpp
|       |   websocket_client.cpp
|       |   websocket_client.h
|       |
|       +---gui
|       |       main_window.cpp
|       |       main_window.h
|       |       main_window.ui
|       |
|       +---logger
|       |       logger.cpp
|       |       logger.h
|       |       QtTextEditSink.h
|       |
|       \---protobuf
|               can_frame.proto
|
\---DBCParserServer
    |   CMakeLists.txt
    |   vcpkg.json
    |
    \---src
        |   main.cpp
        |
        +---gui
        |       mainwindow.cpp
        |       mainwindow.h
        |       mainwindow.ui
        |
        +---logger
        |       logger.cpp
        |       logger.h
        |       QtTextEditSink.h
        |
        +---protobuf
        |       can_frame.proto
        |
        \---websocketserver
                websocketserver.cpp
                websocketserver.h
```

------

## 三、构建与运行

### 1. 环境要求

- Qt 6
- C++20编译器（GCC+MinGW）
- CMake ≥ 3.19
- vcpkg（已启用清单模式）

------

### 2. vcpkg 依赖管理（清单模式）

本项目使用 **vcpkg 清单模式（manifest mode）** 自动管理依赖，无需手动执行 `vcpkg install`。

#### （1）项目根目录包含：

```
vcpkg.json
```

示例内容：

```json
{
  "name": "dbc-parser",
  "version": "1.0.0",
  "dependencies": [
    "protobuf"
  ]
}
```

#### （2）Qt Creator 配置

在 Qt Creator 中完成以下配置：

1. 打开：

   ```
   帮助 → 关于插件 → 勾选vcpkg
   ```

   ```
   首选项 → CMake → Vcpkg → 设置路径;
   首选项 → CMake → 概要 → 勾选Package manager auto setup
   ```

   ```
   项目 → CMake → Intial Configuration → 添加VCPKG_HOST_TRIPLET=x64-mingw-dynamic和-VCPKG_TARGET_TRIPLET=x64-mingw-dynamic
   ```

#### （3）自动安装依赖

当执行 CMake 配置时：

```
CMake Configure（首次）
```

vcpkg 会自动：

```
读取 vcpkg.json → 下载 → 编译 → 集成 protobuf
```

无需手动执行任何安装命令。



### 3. 构建项目

在 Qt Creator 中：

```
打开项目 → 配置 Kit → 点击构建
```

------

### 4. 运行程序

```bash
./DBCParser.exe
```

------

## 四、DBC 文件来源

本项目使用公开 DBC 示例文件demo.dbc和bmw_e9x_e8x.dbc，来源GitHub仓库：

```apl
https://github.com/rundekugel/DBC-files
```



## 五、使用说明

### 1. 加载 DBC

- 启动程序
- 点击“浏览”选择文件
- 点击“解析”后在 TreeView 中展示 Message / Signal

------

### 2. 输入信号值

- 在表格最后一列 **“物理值”** 输入数据

- 系统自动根据：

  ```text
  Raw = (Physical - Offset) / Factor
  ```

  进行换算

------

### 3. 发送 CAN 帧

- 选中某个 Message 行
- 点击“发送”按钮
- 系统执行：
  - 收集该 Message 下所有 Signal
  - 统一打包 CAN Data
  - 通过 WebSocket 发送

------

## 六、WebSocket 服务端

### 1. 默认连接地址

```apl
ws://127.0.0.1:12345
```

------

## 七、Protobuf 说明（加分项）

### 1. proto 定义

```protobuf
syntax = "proto3";

message SignalMsg {
  string name = 1;
  double physical_value = 2;
  string unit = 3;
  int64 raw_value = 4;
}

message CanFrame {
  uint32 message_id = 1;
  string message_name = 2;

  bytes data = 3;           // CAN原始数据

  repeated SignalMsg signal = 4;
}
```

------

### 2. 序列化流程

```text
Signal输入
   ↓
Physical → Raw
   ↓
CAN打包（Data[8]）
   ↓
构建 Protobuf
   ↓
WebSocket 发送（二进制）
```

------

## 八、项目亮点

- 完整实现 DBC → CAN 数据链路
- 支持 Intel / Motorola 位级打包
- Qt Model/View 架构设计
- WebSocket 实时通信
- Protobuf 高效序列化（加分项）
- UI 可交互（信号输入 + 发送）
- CAN 日志记录面板

------

## 九、后续可扩展

- 多 Message 周期发送（定时器）
- WebSocket 自动重连
- 支持 JSON / Protobuf 切换

------

## 十、总结

本项目实现了从 **DBC 文件解析 → 信号编辑 → CAN 帧构造 → 网络发送** 的完整流程，具备良好的工程结构与扩展能力，可用于：

- CAN 协议学习
- 上位机开发

------