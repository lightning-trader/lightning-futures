# lightning-futures

## 介绍

lightning-futures 是一个高性能的期货交易系统框架，提供市场数据获取、交易执行、策略开发等核心功能。该项目支持多种交易接口，包括 CTP 和 TAP，适用于高频交易、算法交易等场景。

## 软件架构

lightning-futures 采用模块化架构，主要包括以下几个模块：

- **Adapter**: 适配不同交易接口（CTP, TAP）的实现。
- **Core**: 提供核心功能，如K线生成、数据通道等。
- **Framework**: 提供策略开发框架。
- **Logger**: 日志记录模块。
- **Share**: 公共组件，如参数解析、CSV操作、内存池等。
- **Simulator**: 回测支持模块，提供模拟交易和数据加载功能。

## 线程模型

该项目使用多线程处理不同任务，确保交易、市场数据接收和策略逻辑之间的高效隔离和协作。主要线程包括：

- **Market Thread**: 负责接收和处理市场行情数据。
- **Trading Thread**: 负责订单的发送和交易处理。
- **Strategy Thread**: 负责策略逻辑的执行。
- **Logger Thread**: 负责日志的异步写入。

## 使用文档

### 编译构建

确保你已安装 CMake 和编译工具，然后执行以下命令：

```bash
mkdir build
cd build
cmake ..
make
```

### 运行示例

在 `src/example` 中，有多个演示程序，展示如何使用框架开发交易策略。例如，`orderflow_strategy` 和 `arbitrage_strategy` 展示了基于不同数据接收模式的策略实现。

```bash
./runtime_demo
```

### 配置

配置文件位于 `bin/config/` 目录下，主要配置包括：

- `runtime_ctpdev.ini`: CTP 接口运行时配置。
- `runtime_tapdev.ini`: TAP � interface runtime 配置。
- `contract.csv`: 合约配置文件。
- `alltrading_section.csv`: 交易时间段配置。

### 策略开发

策略开发基于 `lt::hft::strategy` 接口，用户可通过继承该类并重写以下关键函数实现策略：

- `on_init`: 策略初始化。
- `on_tick`: 接收行情 tick 数据。
- `on_bar`: 接收 K 线数据。
- `on_entrust`, `on_trade`, `on_cancel`: 订单相关回调。
- `on_error`: 错误处理。
- `on_destroy`: �