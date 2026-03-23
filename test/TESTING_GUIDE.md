# Lightning Futures 测试指南

## 概述

本项目已添加自动化测试框架，包含以下测试文件：

### 新增测试文件

1. **test_memory_pool.cpp** - 增强版内存池测试
   - 基本功能测试（分配、释放、重用）
   - 边界情况测试（无效参数、溢出）
   - 内存碎片测试
   - 压力测试（1000+ 次分配/释放）
   - pool_allocator 与 STL 容器配合测试

2. **test_basic_types_edge.cpp** - 基本类型边界测试
   - position_cell 边界测试
   - position_info 计算测试
   - tick_info 盘口测试
   - code_t 合约代码解析测试
   - bar_info 订单流测试
   - order_info 开平方向测试
   - market_info 成交量分布测试
   - tape_info 盘口信息测试
   - symbol_t 期权合约解析测试

3. **test_time_section.cpp** - 交易时间段测试
   - 配置文件解析测试
   - 交易时间判断测试
   - 边界时间点测试
   - next_open_time() 测试

4. **test_trader_simulator_extended.cpp** - 交易模拟器扩展测试
   - 基本功能测试
   - 下单/撤单测试
   - 持仓和订单查询测试
   - tick 推送测试
   - 跨日测试
   - 边界情况测试

5. **test_string_helper.cpp** - 字符串工具函数测试
   - split() 分割函数测试
   - to_string() 转换函数测试（多种类型）
   - contains() 包含判断测试
   - join() 连接函数测试
   - format() 格式化函数测试
   - extract_to_string() 提取转换测试

6. **test_time_utils.cpp** - 时间工具函数测试
   - make_date/make_time/make_datetime 日期时间构造测试
   - get_day_begin/get_day_time 获取日时间测试
   - make_daytm/make_seqtm 日内时间/有序时间构造测试
   - get_uint_day/get_daytm 时间解析测试
   - datetime_to_string/seqtm_to_string 时间格式化测试
   - daytm_offset/time_forward/time_back 时间偏移测试
   - section_daytm_snap 时间取整测试
   - is_same_day/is_same_week/is_same_month/is_same_year 时间比较测试
   - get_week_begin/get_month_begin/get_year_begin 时间边界测试

7. **test_ringbuffer.cpp** - 环形缓冲区测试
   - isEmpty/isFull 空/满判断测试
   - insert/remove 插入删除测试
   - peek 查看首元素测试
   - at 访问指定位置测试
   - producerClear/consumerClear 清除测试
   - readAvailable/writeAvailable 可用数量测试
   - writeBuff/readBuff 批量读写测试
   - 指针插入删除测试

8. **test_event_center.cpp** - 事件中心测试
   - queue_event_source 队列事件源测试
   - direct_event_source 直接事件源测试
   - 事件处理器添加/清除测试
   - 多处理器测试
   - 不同类型事件测试
   - event_data 结构测试

9. **test_crontab_scheduler.cpp** - 定时调度器测试
   - add_schedule 添加定时任务测试
   - set_work_days 设置工作日测试
   - set_callback 设置回调测试
   - polling 轮询测试
   - 多定时任务测试
   - 回调函数调用测试

10. **test_stream_buffer.cpp** - 流缓冲区测试
    - stream_carbureter 基本功能测试
    - stream_carbureter clear 清除测试
    - stream_carbureter read 读取测试
    - stream_extractor 基本功能测试
    - stream_extractor reset 重置测试
    - stream_extractor extract 提取测试
    - stream_extractor out 输出测试
    - 多次追加/提取测试
    - 边界情况测试

### 原有测试文件

- test_basic_types.cpp - 基本类型测试
- test_basic_utils.cpp - 工具函数测试
- test_params.cpp - 参数解析测试
- test_trading_context.cpp - 交易上下文测试
- test_trader_simulator.cpp - 交易模拟器测试
- test_fix.cpp - FIX 协议测试
- test_position_fix.cpp - 持仓 FIX 测试
- test_simulator_fix.cpp - 模拟器 FIX 测试
- test_compiler.cpp - 编译器特性测试

## 编译和运行测试

### Windows (批处理)

```batch
cd test
run_tests.bat
```

### Windows (PowerShell)

```powershell
cd test
.\run_tests.ps1
```

### CMake (跨平台)

```bash
cd test
mkdir build
cd build
cmake ..
cmake --build .
ctest
```

## 测试框架特性

1. **统一的测试宏**
   ```cpp
   ASSERT_TRUE(condition, "message")      // 条件为真时通过
   ASSERT_FALSE(condition, "message")     // 条件为假时通过
   ASSERT_EQ(expected, actual, "message") // 值相等时通过
   ASSERT_THROW(expr, type, "message")    // 抛出指定异常时通过
   ```

2. **测试结果统计**
   - 自动统计通过/失败数量
   - 返回码：0=全部通过，1=有失败

3. **边界测试**
   - 空值/零值测试
   - 最大值测试
   - 无效参数测试
   - 异常处理测试

## 已修复的 BUG 和优化

### 内存安全修复

1. **memory_pool.hpp** - 内存泄漏风险
   - 问题：析构函数未释放已分配但未归还的块
   - 修复：添加 `_allocated_blocks` 集合追踪分配的块，析构时释放

2. **basic_types.hpp** - 除零风险
   - 问题：`get_buy_volume()` 和 `get_sell_volume()` 中 `detail_density` 可能为 0
   - 修复：添加 `detail_density <= 0` 检查

### 并发安全修复

3. **mpsc_queue.hpp** - 竞态条件
   - 问题：`enqueue_batch()` 和 `size()` 缺少锁保护
   - 修复：添加 `std::lock_guard` 保护

### 数组越界修复

4. **time_section.cpp** - 数组越界
   - 问题：访问 `times[2]` 和 `times[3]` 时未检查大小
   - 修复：添加 `times.size() >= 4` 检查

### 类型转换修复

5. **trader_simulator.cpp** - 类型转换和计算错误
   - 问题：`handle_entrust()` 中 `interest_delta` 计算类型转换错误
   - 问题：`unfrozen_deduction()` 中 `last_volume` 类型不匹配
   - 修复：使用正确的类型转换和参数类型

### 拼写错误修复

6. **多处拼写错误**
   - `OS_CANELED` → `OS_CANCELED`
   - `frist_one` → `first_one`
   - `monery` → `money` (shared_types.h)
   - `reuslt` → `result` (bar_generator.cpp)
   - `valume` → `volume` (bar_generator.cpp)

## 添加新测试

要添加新的测试文件，请遵循以下模板：

```cpp
#include <iostream>
#include <vector>
#include <cassert>
#include "../src/include/basic_define.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(condition, message) \
    do { \
        if (condition) { \
            std::cout << "   [PASS] " << message << std::endl; \
            tests_passed++; \
        } else { \
            std::cout << "   [FAIL] " << message << std::endl; \
            tests_failed++; \
        } \
    } while(0)

void test_your_feature() {
    std::cout << "\n=== 测试你的功能 ===" << std::endl;
    
    // 测试代码
    ASSERT_TRUE(your_condition, "描述你的测试");
}

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "   你的测试模块名称" << std::endl;
    std::cout << "============================================" << std::endl;
    
    test_your_feature();
    
    std::cout << "\n============================================" << std::endl;
    std::cout << "   测试结果汇总" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "   通过：" << tests_passed << std::endl;
    std::cout << "   失败：" << tests_failed << std::endl;
    std::cout << "============================================" << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}
```

## 持续集成

建议将测试集成到 CI/CD 流程中：

```yaml
# 示例 GitHub Actions 配置
name: Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v2
      - name: Setup MSVC
        uses: ilammy/msvc-dev-cmd@v1
      - name: Run Tests
        run: cd test && .\run_tests.ps1