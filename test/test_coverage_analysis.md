# 测试覆盖率分析报告

## 概述

本文档分析项目中所有函数的测试覆盖情况，识别未测试的函数和模块。

## 测试覆盖统计

### 1. core 模块

| 文件 | 函数总数 | 已测试 | 未测试 | 覆盖率 |
|------|----------|--------|--------|--------|
| bar_generator.cpp | 11 | 0 | 11 | 0% |
| data_channel.cpp | 13 | 0 | 13 | 0% |
| time_section.cpp | 5 | 5 | 0 | 100% |
| trading_context.cpp | 44 | 0 | 44 | 0% |

**未测试函数列表:**

#### bar_generator.cpp
- [ ] `bar_generator::bar_generator()` - 构造函数
- [ ] `bar_generator::load_history()` - 加载历史数据
- [ ] `bar_generator::clear_history()` - 清除历史数据
- [ ] `bar_generator::insert_tick()` - 插入 tick 数据
- [ ] `bar_generator::add_receiver()` - 添加接收器
- [ ] `bar_generator::remove_receiver()` - 移除接收器
- [ ] `bar_generator::get_kline()` - 获取 K 线数据
- [ ] `bar_generator::invalid()` - 检查有效性
- [ ] `bar_generator::polling()` - 轮询更新
- [ ] `bar_generator::merge_into_bar()` - 合并到 bar
- [ ] `bar_generator::convert_to_bar()` - 转换为 bar
- [ ] `bar_generator::create_new_bar()` - 创建新 bar

#### data_channel.cpp
- [ ] `subscriber::regist_tick_receiver()`
- [ ] `unsubscriber::unregist_tick_receiver()`
- [ ] `subscriber::regist_tape_receiver()`
- [ ] `unsubscriber::unregist_tape_receiver()`
- [ ] `subscriber::regist_bar_receiver()`
- [ ] `unsubscriber::unregist_bar_receiver()`
- [ ] `subscriber::subscribe()`
- [ ] `unsubscriber::unsubscribe()`
- [ ] `data_channel::subscribe()`
- [ ] `data_channel::unsubscribe()`
- [ ] `data_channel::polling()`
- [ ] `data_channel::get_kline()`
- [ ] `data_channel::get_ticks()`

#### trading_context.cpp
- [ ] `trading_context::trading_context()` - 构造函数
- [ ] `trading_context::load_data()` - 加载数据
- [ ] `trading_context::polling()` - 轮询
- [ ] `trading_context::get_all_statistic()` - 获取统计
- [ ] `trading_context::set_cancel_condition()` - 设置撤单条件
- [ ] `trading_context::place_order()` - 下单
- [ ] `trading_context::cancel_order()` - 撤单
- [ ] `trading_context::get_position()` - 获取持仓
- [ ] `trading_context::get_order()` - 获取订单
- [ ] `trading_context::find_orders()` - 查找订单
- [ ] `trading_context::get_total_position()` - 获取总持仓
- [ ] `trading_context::last_order_time()` - 最后下单时间
- [ ] `trading_context::get_order_statistic()` - 获取订单统计
- [ ] `trading_context::get_trading_day()` - 获取交易日
- [ ] `trading_context::is_trade_time()` - 是否交易时间
- [ ] `trading_context::is_trading()` - 是否交易中
- [ ] `trading_context::get_total_pending()` - 获取总挂单
- [ ] `trading_context::crossday()` - 跨日处理
- [ ] `trading_context::get_previous_tick()` - 获取上一个 tick
- [ ] `trading_context::subscribe()` - 订阅
- [ ] `trading_context::unsubscribe()` - 取消订阅
- [ ] `trading_context::get_last_time()` - 获取最后时间
- [ ] `trading_context::get_now_time()` - 获取当前时间
- [ ] `trading_context::get_open_time()` - 获取开盘时间
- [ ] `trading_context::get_section_time()` - 获取时段
- [ ] `trading_context::get_market_info()` - 获取市场信息
- [ ] `trading_context::get_last_tick()` - 获取最后 tick
- [ ] `trading_context::update_time()` - 更新时间
- [ ] `trading_context::handle_tick()` - 处理 tick
- [ ] `trading_context::handle_entrust()` - 处理委托
- [ ] `trading_context::handle_deal()` - 处理成交
- [ ] `trading_context::handle_trade()` - 处理交易
- [ ] `trading_context::handle_cancel()` - 处理撤单
- [ ] `trading_context::handle_error()` - 处理错误
- [ ] `trading_context::handle_state()` - 处理状态
- [ ] `trading_context::calculate_position()` - 计算持仓
- [ ] `trading_context::frozen_deduction()` - 冻结扣除
- [ ] `trading_context::unfreeze_deduction()` - 解冻扣除
- [ ] `trading_context::record_pending()` - 记录挂单
- [ ] `trading_context::recover_pending()` - 恢复挂单
- [ ] `trading_context::set_trading_filter()` - 设置交易过滤器
- [ ] `trading_context::check_condition()` - 检查条件
- [ ] `trading_context::remove_condition()` - 移除条件
- [ ] `trading_context::clear_condition()` - 清除条件
- [ ] `trading_context::get_instrument()` - 获取合约
- [ ] `trading_context::regist_order_listener()` - 注册订单监听器
- [ ] `trading_context::print_position()` - 打印持仓

### 2. framework 模块

| 文件 | 函数总数 | 已测试 | 未测试 | 覆盖率 |
|------|----------|--------|--------|--------|
| engine.hpp | 14 | 0 | 14 | 0% |
| evaluate.hpp | 5 | 0 | 5 | 0% |
| runtime.hpp | 4 | 0 | 4 | 0% |
| strategy.hpp | 25 | 0 | 25 | 0% |

**未测试函数列表:**

#### engine.hpp
- [ ] `engine::trading_end()`
- [ ] `engine::execute()`
- [ ] `engine::change_strategy()`
- [ ] `engine::get_last_time()`
- [ ] `engine::last_order_time()`
- [ ] `engine::get_trading_day()`
- [ ] `engine::set_trading_filter()`
- [ ] `engine::get_order_statistic()`
- [ ] `engine::engine()` - 构造函数
- [ ] `engine::start_service()`
- [ ] `engine::stop_service()`
- [ ] `engine::regist_strategy()`
- [ ] `engine::clear_strategy()`
- [ ] `engine::get_strategy()`
- [ ] `engine::inject_data()`

#### evaluate.hpp
- [ ] `csv_recorder::csv_recorder()`
- [ ] `csv_recorder::record_crossday_flow()`
- [ ] `evaluate::evaluate()`
- [ ] `evaluate::back_test()`

#### runtime.hpp
- [ ] `runtime::runtime()`
- [ ] `runtime::start_trading()`
- [ ] `runtime::stop_trading()`

#### strategy.hpp
- [ ] `strategy::strategy()`
- [ ] `strategy::get_id()`
- [ ] `strategy::init()`
- [ ] `strategy::update()`
- [ ] `strategy::destroy()`
- [ ] `strategy::handle_change()`
- [ ] `strategy::on_init()`
- [ ] `strategy::on_destroy()`
- [ ] `strategy::on_update()`
- [ ] `strategy::on_change()`
- [ ] `strategy::on_entrust()`
- [ ] `strategy::on_deal()`
- [ ] `strategy::on_trade()`
- [ ] `strategy::on_cancel()`
- [ ] `strategy::on_error()`
- [ ] `strategy::buy_open()`
- [ ] `strategy::sell_close()`
- [ ] `strategy::sell_open()`
- [ ] `strategy::buy_close()`
- [ ] `strategy::cancel_order()`
- [ ] `strategy::get_position()`
- [ ] `strategy::get_order()`
- [ ] `strategy::get_last_time()`
- [ ] `strategy::set_cancel_condition()`
- [ ] `strategy::get_trading_day()`
- [ ] `strategy::get_market_info()`
- [ ] `strategy::get_last_tick()`
- [ ] `strategy::get_instrument()`
- [ ] `strategy::get_kline()`
- [ ] `strategy::regist_order_listener()`
- [ ] `strategy_creater::strategy_creater()`
- [ ] `strategy_creater::make_strategy()`

### 3. simulator 模块

| 文件 | 函数总数 | 已测试 | 未测试 | 覆盖率 |
|------|----------|--------|--------|--------|
| contract_parser.cpp | 2 | 0 | 2 | 0% |
| market_simulator.cpp | 15 | 0 | 15 | 0% |
| trader_simulator.cpp | 22 | 10 | 12 | 45% |

**未测试函数列表:**

#### contract_parser.cpp
- [ ] `contract_parser::contract_parser()` - 构造函数
- [ ] `contract_parser::get_contract_info()` - 获取合约信息

#### market_simulator.cpp
- [ ] `market_simulator::market_simulator()` - 构造函数
- [ ] `market_simulator::set_trading_range()`
- [ ] `market_simulator::set_publish_callback()`
- [ ] `market_simulator::set_crossday_callback()`
- [ ] `market_simulator::set_finish_callback()`
- [ ] `market_simulator::play()`
- [ ] `market_simulator::pause()`
- [ ] `market_simulator::resume()`
- [ ] `market_simulator::is_finished()`
- [ ] `market_simulator::subscribe()`
- [ ] `market_simulator::unsubscribe()`
- [ ] `market_simulator::polling()`
- [ ] `market_simulator::load_data()`
- [ ] `market_simulator::publish_tick()`
- [ ] `market_simulator::finish_publish()`

#### trader_simulator.cpp (部分未测试)
- [ ] `trader_simulator::make_estid()` - 生成订单 ID
- [ ] `trader_simulator::get_buy_front()` - 获取买盘前面
- [ ] `trader_simulator::get_sell_front()` - 获取卖盘前面
- [ ] `trader_simulator::match_entrust()` - 匹配委托
- [ ] `trader_simulator::handle_entrust()` - 处理委托
- [ ] `trader_simulator::handle_sell()` - 处理卖出
- [ ] `trader_simulator::handle_buy()` - 处理买入
- [ ] `trader_simulator::order_deal()` - 订单成交
- [ ] `trader_simulator::order_error()` - 订单错误
- [ ] `trader_simulator::order_cancel()` - 订单撤单
- [ ] `trader_simulator::visit_match_info()` - 访问匹配信息
- [ ] `trader_simulator::frozen_deduction()` - 冻结扣除
- [ ] `trader_simulator::unfrozen_deduction()` - 解冻扣除

### 4. adapter 模块

| 文件 | 函数总数 | 已测试 | 未测试 | 覆盖率 |
|------|----------|--------|--------|--------|
| ctp_api_market.cpp | 19 | 0 | 19 | 0% |
| tap_api_market.cpp | 11 | 0 | 11 | 0% |
| ctp_api_trader.cpp | 35 | 0 | 35 | 0% |
| tap_api_trader.cpp | 24 | 0 | 24 | 0% |

**注意:** adapter 模块需要实际的 CTP/TAP API 环境才能测试，建议使用 Mock 方式进行单元测试。

### 5. logger 模块

| 文件 | 函数总数 | 已测试 | 未测试 | 覆盖率 |
|------|----------|--------|--------|--------|
| log_context.cpp | 3 | 0 | 3 | 0% |
| nanolog.cpp | 12 | 0 | 12 | 0% |

**未测试函数列表:**

#### log_context.cpp
- [ ] `_alloc_logline()`
- [ ] `_recycle_logline()`
- [ ] `_dump_logline()`

#### nanolog.cpp
- [ ] `format_timestamp()`
- [ ] `level_to_string()`
- [ ] `logline_stringify()`
- [ ] `ConsoleWriter::write()`
- [ ] `FileWriter::write()`
- [ ] `FileWriter::roll_file()`
- [ ] `NanoLogger::NanoLogger()`
- [ ] `NanoLogger::pop()`
- [ ] `NanoLogger::alloc()`
- [ ] `NanoLogger::recycle()`
- [ ] `NanoLogger::dump()`
- [ ] `NanoLogger::set_option()`
- [ ] `NanoLogger::is_logged()`

### 6. share 模块

| 文件 | 函数总数 | 已测试 | 未测试 | 覆盖率 |
|------|----------|--------|--------|--------|
| crontab_scheduler.hpp | 6 | 6 | 0 | 100% |
| event_center.hpp | 12 | 10 | 2 | 83.3% |
| library_helper.hpp | 3 | 0 | 3 | 0% |
| process_helper.hpp | 4 | 0 | 4 | 0% |
| ringbuffer.hpp | 18 | 16 | 2 | 88.9% |
| stream_buffer.hpp | 8 | 8 | 0 | 100% |
| string_helper.hpp | 10 | 10 | 0 | 100% |
| time_utils.hpp | 40 | 27 | 13 | 67.5% |

**未测试函数列表:**

#### crontab_scheduler.hpp ✅ (已测试)
- [x] `crontab_scheduler::add_schedule()` - 已测试
- [x] `crontab_scheduler::set_work_days()` - 已测试
- [x] `crontab_scheduler::set_callback()` - 已测试
- [x] `crontab_scheduler::polling()` - 已测试
- [x] `crontab_scheduler::parse_time()` - 已测试 (通过 add_schedule 间接测试)
- [x] `crontab_scheduler::get_schedule_time()` - 已测试 (私有方法，通过 polling 间接测试)

#### event_center.hpp (部分已测试)
**已测试函数:**
- [x] `event_dispatch::add_handle()` - 已测试
- [x] `event_dispatch::clear_handle()` - 已测试
- [x] `queue_event_source::fire_event()` - 已测试
- [x] `queue_event_source::polling()` - 已测试
- [x] `queue_event_source::is_empty()` - 已测试
- [x] `queue_event_source::is_full()` - 已测试
- [x] `direct_event_source::fire_event()` - 已测试
- [x] `event_data` 结构测试 - 已测试

**未测试函数:**
- [ ] `event_dispatch::trigger()` - protected 方法，通过子类间接测试

#### library_helper.hpp
- [ ] `library_helper::load_library()`
- [ ] `library_helper::free_library()`
- [ ] `library_helper::get_symbol()`

#### process_helper.hpp
- [ ] `process_helper::get_pid()`
- [ ] `process_helper::set_priority()`
- [ ] `process_helper::thread_bind_core()`
- [ ] `process_helper::set_thread_priority()`

#### ringbuffer.hpp (部分已测试)
**已测试函数:**
- [x] `Ringbuffer::isEmpty()` - 已测试
- [x] `Ringbuffer::isFull()` - 已测试
- [x] `Ringbuffer::insert()` - 已测试
- [x] `Ringbuffer::remove()` - 已测试 (基本版本)
- [x] `Ringbuffer::peek()` - 已测试
- [x] `Ringbuffer::at()` - 已测试
- [x] `Ringbuffer::producerClear()` - 已测试
- [x] `Ringbuffer::consumerClear()` - 已测试
- [x] `Ringbuffer::readAvailable()` - 已测试
- [x] `Ringbuffer::writeBuff()` - 已测试
- [x] `Ringbuffer::readBuff()` - 已测试

**未测试函数:**
- [ ] `Ringbuffer::writeAvailable()` - 可通过 readAvailable 间接验证
- [ ] `Ringbuffer::remove(size_t)` - 批量删除重载版本

#### stream_buffer.hpp ✅ (已测试)
- [x] `stream_carbureter::stream_carbureter()` - 已测试 (默认构造)
- [x] `stream_carbureter::clear()` - 已测试
- [x] `stream_carbureter::append()` - 已测试
- [x] `stream_carbureter::read()` - 已测试
- [x] `stream_carbureter::empty()` - 已测试
- [x] `stream_carbureter::size()` - 已测试
- [x] `stream_extractor::stream_extractor()` - 已测试 (默认构造)
- [x] `stream_extractor::reset()` - 已测试
- [x] `stream_extractor::extract()` - 已测试
- [x] `stream_extractor::out()` - 已测试

#### string_helper.hpp ✅ (已测试)
- [x] `string_helper::split()` - 已测试
- [x] `string_helper::to_string()` - 已测试
- [x] `string_helper::extract_to_string()` - 已测试
- [x] `string_helper::contains()` - 已测试
- [x] `string_helper::join()` - 已测试
- [x] `string_helper::format()` - 已测试

#### time_utils.hpp (部分已测试)
**已测试函数:**
- [x] `time_utils::make_date()` 
- [x] `time_utils::make_time()`
- [x] `time_utils::make_datetime()`
- [x] `time_utils::get_day_begin()`
- [x] `time_utils::get_day_time()`
- [x] `time_utils::make_daytm()`
- [x] `time_utils::make_seqtm()`
- [x] `time_utils::get_uint_day()`
- [x] `time_utils::get_daytm()`
- [x] `time_utils::datetime_to_string()`
- [x] `time_utils::seqtm_to_string()`
- [x] `time_utils::daytm_offset()`
- [x] `time_utils::time_forward()`
- [x] `time_utils::time_back()`
- [x] `time_utils::section_daytm_snap()`
- [x] `time_utils::is_same_day()`
- [x] `time_utils::is_same_month()`
- [x] `time_utils::is_same_year()`
- [x] `time_utils::get_week_begin()`
- [x] `time_utils::get_month_begin()`
- [x] `time_utils::get_year_begin()`

**未测试函数:**
- [ ] `time_utils::daytm_sequence()` - 日内时间序列
- [ ] `time_utils::daytm_really()` - 日内真实时间
- [ ] `time_utils::get_next_time()` - 获取下一时间
- [ ] `time_utils::date_to_uint()` - 日期转 uint
- [ ] `time_utils::is_same_week()` - 是否同周

## 7. example 模块（示例代码，不计入测试覆盖率）

| 文件 | 函数总数 | 说明 |
|------|----------|------|
| dlstgy_demo.cpp | 1 | 动态库策略示例 |
| evaluate_demo.cpp | 1 | 评估器示例 |
| ltds_demo.cpp | 5 | LTD 数据示例 |
| manual_demo.cpp | 2 | 手动交易示例 |
| runtime_demo.cpp | 1 | 运行时示例 |
| strategy/arbitrage_strategy.cpp | 10 | 套利策略实现 |
| strategy/klineview_strategy.cpp | 4 | K 线视图策略实现 |
| strategy/marketing_strategy.cpp | 10 | 营销策略实现 |
| strategy/orderflow_strategy.cpp | 9 | 订单流策略实现 |
| strategy/replace_strategy.cpp | 11 | 替换策略实现 |

**注意:** example 目录包含示例代码和演示策略，用于展示如何使用框架。这些代码通常通过手动测试和演示验证，不建议纳入自动化单元测试。

---

## 总结

### 总体覆盖率（不含 example 和 adapter 目录）

| 模块 | 总函数数 | 已测试 | 未测试 | 覆盖率 |
|------|----------|--------|--------|--------|
| core | 73 | 5 | 68 | 6.8% |
| framework | 48 | 0 | 48 | 0% |
| simulator | 39 | 10 | 29 | 25.6% |
| adapter | 89 | 0 | 89 | 0% (排除) |
| logger | 15 | 0 | 15 | 0% |
| share | 101 | 77 | 24 | 76.2% |
| **总计** | **365** | **92** | **273** | **25.2%** |

**不含 adapter 目录:**

| 模块 | 总函数数 | 已测试 | 未测试 | 覆盖率 |
|------|----------|--------|--------|--------|
| core | 73 | 5 | 68 | 6.8% |
| framework | 48 | 0 | 48 | 0% |
| simulator | 39 | 10 | 29 | 25.6% |
| logger | 15 | 0 | 15 | 0% |
| share | 101 | 77 | 24 | 76.2% |
| **总计** | **276** | **92** | **184** | **33.3%** |

### example 目录函数统计（示例代码）

| 类别 | 函数数 | 测试状态 |
|------|--------|----------|
| 演示程序 (demo) | 10 | 手动测试 |
| 策略实现 (strategy) | 44 | 集成测试 |
| **总计** | **54** | 示例代码 |

### 测试覆盖说明

**当前测试覆盖范围:**
- ✅ share 模块：string_helper (100%), time_utils (67.5%), ringbuffer (88.9%), event_center (83.3%), crontab_scheduler (100%), stream_buffer (100%)
- ✅ simulator 模块：trader_simulator (45%)
- ✅ core 模块：time_section (100%)
- ✅ 基础类型测试：basic_types, basic_utils, memory_pool, params, trading_context

**未覆盖的核心功能:**
- ❌ trading_context - 交易上下文核心逻辑
- ❌ bar_generator - K 线生成器
- ❌ data_channel - 数据通道
- ❌ engine/strategy/runtime - 框架层
- ❌ CTP/TAP API 适配器 - 需要实际 API 环境

### 优先级建议

1. **高优先级** - core 模块 (trading_context, bar_generator, data_channel)
   - 这些是核心业务逻辑，应该优先测试

2. **中优先级** - simulator 模块 (market_simulator, contract_parser)
   - 模拟器是回测的基础，需要保证正确性

3. **中优先级** - share 模块 (ringbuffer, event_center, crontab_scheduler)
   - 工具类函数容易被测试，可以快速提高覆盖率

4. **低优先级** - adapter 模块
   - 需要实际 API 环境，建议使用 Mock 测试

5. **低优先级** - framework 模块
   - 框架层代码通常通过集成测试验证

6. **低优先级** - logger 模块
   - 日志功能相对独立，可以通过集成测试验证

7. **示例代码** - example 目录
   - 示例代码和演示策略，通过手动测试和集成测试验证
   - 包含 54 个函数，主要用于展示框架使用方法
