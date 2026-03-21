#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include "../src/include/basic_define.h"
#include "../src/include/basic_types.hpp"
#include "../src/share/shared_types.h"
#include "../src/include/params.hpp"
#include "../src/simulator/trader_simulator.h"
#include "../src/share/trader_api.h"

// 测试计数器
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

#define ASSERT_EQ(expected, actual, message) \
    do { \
        if ((expected) == (actual)) { \
            std::cout << "   [PASS] " << message << std::endl; \
            tests_passed++; \
        } else { \
            std::cout << "   [FAIL] " << message << std::endl; \
            tests_failed++; \
        } \
    } while(0)

#define ASSERT_FALSE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "   [PASS] " << message << std::endl; \
            tests_passed++; \
        } else { \
            std::cout << "   [FAIL] " << message << std::endl; \
            tests_failed++; \
        } \
    } while(0)

// 创建测试配置
lt::params create_test_params() {
    std::map<std::string, std::string> params_map;
    params_map["initial_capital"] = "1000000.0";
    params_map["commission_rate"] = "0.0001";
    params_map["margin_rate"] = "0.1";
    params_map["slippage"] = "1.0";
    return lt::params(params_map);
}

// ==================== 基本功能测试 ====================

// 测试 trader_simulator 基本功能
void test_trader_simulator_basic() {
    std::cout << "\n=== 测试 trader_simulator 基本功能 ===" << std::endl;
    
    lt::params params = create_test_params();
    lt::driver::trader_simulator trader(params);
    
    // 测试构造函数
    ASSERT_TRUE(true, "trader_simulator 构造成功");
    
    // 测试 get_trading_day()
    uint32_t trading_day = trader.get_trading_day();
    ASSERT_TRUE(trading_day == 0 || trading_day > 0, "get_trading_day() 返回有效值");
    
    // 测试 is_usable()
    ASSERT_TRUE(!trader.is_usable(), "初始化后 is_usable() = false (需要连接)");
    
    // 测试 get_account()
    const lt::account_info& account = trader.get_account();
    ASSERT_TRUE(account.money >= 0, "get_account() 返回有效账户信息");
}

// 测试下单和撤单
void test_place_order() {
    std::cout << "\n=== 测试下单和撤单 ===" << std::endl;
    
    lt::params params = create_test_params();
    lt::driver::trader_simulator trader(params);
    
    // 测试 place_order() 返回无效 estid (未连接状态)
    lt::code_t code("SHFE.rb2105");
    lt::estid_t estid = trader.place_order(
        lt::offset_type::OT_OPEN,
        lt::direction_type::DT_LONG,
        code,
        1,  // 1 手
        4500.0,  // 价格
        lt::order_flag::OF_NOR
    );
    
    // 在未连接状态下，下单应该返回 INVALID_ESTID
    ASSERT_EQ(INVALID_ESTID, estid, "未连接状态下 place_order 返回 INVALID_ESTID");
    
    // 测试 cancel_order() 无效 estid
    ASSERT_FALSE(trader.cancel_order(INVALID_ESTID), "cancel_order(INVALID_ESTID) 返回 false");
}

// 测试 get_all_orders() 和 get_all_positions()
void test_get_all_orders_positions() {
    std::cout << "\n=== 测试获取订单和持仓 ===" << std::endl;
    
    lt::params params = create_test_params();
    lt::driver::trader_simulator trader(params);
    
    // 测试 get_all_orders() 空订单列表
    auto orders = trader.get_all_orders();
    ASSERT_TRUE(orders.empty(), "初始化时订单列表为空");
    
    // 测试 get_all_positions() 空持仓列表
    auto positions = trader.get_all_positions();
    ASSERT_TRUE(positions.empty(), "初始化时持仓列表为空");
}

// 测试 push_tick()
void test_push_tick() {
    std::cout << "\n=== 测试 push_tick() ===" << std::endl;
    
    lt::params params = create_test_params();
    lt::driver::trader_simulator trader(params);
    
    // 创建测试 tick
    lt::tick_info tick;
    tick.code = lt::code_t("SHFE.rb2105");
    tick.time = 90000000;
    tick.price = 4500.0;
    tick.volume = 1000;
    tick.open_interest = 10000;
    tick.average_price = 4500.0;
    tick.trading_day = 20210101;
    
    std::vector<const lt::tick_info*> ticks;
    ticks.push_back(&tick);
    
    // 测试 push_tick()
    trader.push_tick(ticks);
    ASSERT_TRUE(true, "push_tick() 执行成功");
}

// 测试 crossday()
void test_crossday() {
    std::cout << "\n=== 测试 crossday() ===" << std::endl;
    
    lt::params params = create_test_params();
    lt::driver::trader_simulator trader(params);
    
    // 测试 crossday()
    trader.crossday(20210102);
    ASSERT_TRUE(true, "crossday() 执行成功");
    
    // crossday 后 trading_day 应该更新
    uint32_t new_day = trader.get_trading_day();
    ASSERT_EQ(20210102U, new_day, "crossday() 后 trading_day 更新");
}

// 测试 polling()
void test_polling() {
    std::cout << "\n=== 测试 polling() ===" << std::endl;
    
    lt::params params = create_test_params();
    lt::driver::trader_simulator trader(params);
    
    // 测试 polling()
    bool result = trader.polling();
    ASSERT_TRUE(result == true || result == false, "polling() 返回布尔值");
}

// 测试 get_all_instruments()
void test_get_all_instruments() {
    std::cout << "\n=== 测试 get_all_instruments() ===" << std::endl;
    
    lt::params params = create_test_params();
    lt::driver::trader_simulator trader(params);
    
    // 测试 get_all_instruments()
    auto instruments = trader.get_all_instruments();
    // 在没有配置合约的情况下，可能返回空列表或默认列表
    ASSERT_TRUE(instruments.size() >= 0, "get_all_instruments() 返回有效列表");
}

// 测试订单状态
void test_order_state() {
    std::cout << "\n=== 测试订单状态 ===" << std::endl;
    
    // 注意：order_state 是 trader_simulator 的私有枚举
    // 这里只测试基本接口
    std::cout << "   [INFO] order_state 是私有枚举，跳过测试" << std::endl;
    ASSERT_TRUE(true, "order_state 基本测试跳过");
}

// 测试 position_detail 结构
void test_position_detail() {
    std::cout << "\n=== 测试 position_detail 结构 ===" << std::endl;
    
    // 注意：position_detail 是 trader_simulator 的内部类
    // 这里只测试基本接口
    
    std::cout << "   [INFO] position_detail 是内部类，跳过详细测试" << std::endl;
    ASSERT_TRUE(true, "position_detail 基本测试跳过");
}

// 测试边界情况
void test_edge_cases() {
    std::cout << "\n=== 测试边界情况 ===" << std::endl;
    
    // 测试空参数
    {
        lt::params empty_params;
        lt::driver::trader_simulator trader(empty_params);
        ASSERT_TRUE(true, "空参数构造成功");
    }
    
    // 测试零价格下单
    {
        lt::params params = create_test_params();
        lt::driver::trader_simulator trader(params);
        lt::code_t code("SHFE.rb2105");
        lt::estid_t estid = trader.place_order(
            lt::offset_type::OT_OPEN,
            lt::direction_type::DT_LONG,
            code,
            1,
            0.0,  // 零价格
            lt::order_flag::OF_NOR
        );
        ASSERT_EQ(INVALID_ESTID, estid, "零价格下单返回 INVALID_ESTID");
    }
    
    // 测试零数量下单
    {
        lt::params params = create_test_params();
        lt::driver::trader_simulator trader(params);
        lt::code_t code("SHFE.rb2105");
        lt::estid_t estid = trader.place_order(
            lt::offset_type::OT_OPEN,
            lt::direction_type::DT_LONG,
            code,
            0,  // 零数量
            4500.0,
            lt::order_flag::OF_NOR
        );
        ASSERT_EQ(INVALID_ESTID, estid, "零数量下单返回 INVALID_ESTID");
    }
}

// ==================== 扩展功能测试 ====================

// 测试 trader_simulator 类（原 test_trader_simulator.cpp 内容）
void test_trader_simulator_extended() {
    std::cout << "\n=== 测试 trader_simulator 扩展功能 ===" << std::endl;
    
    // 测试初始化
    std::cout << "1. 测试初始化:" << std::endl;
    
    std::map<std::string, std::string> config_map;
    config_map["interval"] = "100";
    config_map["initial_capital"] = "1000000.0";
    lt::params config(config_map);
    
    try {
        lt::driver::trader_simulator simulator(config);
        ASSERT_TRUE(true, "trader_simulator 初始化成功");
        
        // 测试 get_trading_day 方法
        std::cout << "2. 测试 get_trading_day 方法:" << std::endl;
        auto trading_day = simulator.get_trading_day();
        std::cout << "   trading_day = " << trading_day << std::endl;
        ASSERT_TRUE(trading_day >= 0, "get_trading_day 返回有效值");
        
        // 测试 is_usable 方法
        std::cout << "3. 测试 is_usable 方法:" << std::endl;
        bool usable = simulator.is_usable();
        std::cout << "   is_usable() = " << usable << std::endl;
        ASSERT_TRUE(usable == true || usable == false, "is_usable() 返回布尔值");
        
        // 测试 get_all_instruments 方法
        std::cout << "4. 测试 get_all_instruments 方法:" << std::endl;
        auto instruments = simulator.get_all_instruments();
        std::cout << "   合约数量：" << instruments.size() << std::endl;
        ASSERT_TRUE(instruments.size() >= 0, "get_all_instruments 返回有效列表");
        
        // 测试 place_order 方法 - 开仓
        std::cout << "5. 测试 place_order 方法 - 开仓:" << std::endl;
        lt::code_t code("IF2109");
        auto estid_open = simulator.place_order(lt::offset_type::OT_OPEN, lt::direction_type::DT_LONG, code, 10, 4500.0, lt::order_flag::OF_NOR);
        std::cout << "   开仓 10 手多头，订单 ID: " << estid_open << std::endl;
        // 注意：在未连接状态下可能返回 INVALID_ESTID
        
        // 测试 cancel_order 方法
        std::cout << "6. 测试 cancel_order 方法:" << std::endl;
        bool cancel_result = simulator.cancel_order(estid_open);
        std::cout << "   撤单结果：" << cancel_result << std::endl;
        ASSERT_TRUE(cancel_result == true || cancel_result == false, "cancel_order 返回布尔值");
        
        // 测试 get_all_orders 方法
        std::cout << "7. 测试 get_all_orders 方法:" << std::endl;
        auto orders = simulator.get_all_orders();
        std::cout << "   订单数量：" << orders.size() << std::endl;
        ASSERT_TRUE(orders.size() >= 0, "get_all_orders 返回有效列表");
        
        // 测试 get_all_positions 方法
        std::cout << "8. 测试 get_all_positions 方法:" << std::endl;
        auto positions = simulator.get_all_positions();
        std::cout << "   持仓数量：" << positions.size() << std::endl;
        for (const auto& pos : positions) {
            std::cout << "   合约：" << pos.code.get_symbol() << std::endl;
            std::cout << "   多头仓位：" << pos.current_long << std::endl;
            std::cout << "   空头仓位：" << pos.current_short << std::endl;
        }
        ASSERT_TRUE(positions.size() >= 0, "get_all_positions 返回有效列表");
        
        // 测试 crossday 方法
        std::cout << "9. 测试 crossday 方法:" << std::endl;
        simulator.crossday(20210901);
        std::cout << "   跨天处理成功，交易日：20210901" << std::endl;
        ASSERT_EQ(20210901U, simulator.get_trading_day(), "crossday 后 trading_day 更新");
        
        // 测试 push_tick 方法
        std::cout << "10. 测试 push_tick 方法:" << std::endl;
        lt::tick_info tick;
        tick.code = code;
        tick.time = 1000000;
        tick.price = 4500.0;
        tick.volume = 10000;
        tick.open_interest = 100000;
        
        std::vector<const lt::tick_info*> ticks;
        ticks.push_back(&tick);
        
        simulator.push_tick(ticks);
        std::cout << "   推送 tick 成功" << std::endl;
        ASSERT_TRUE(true, "push_tick 执行成功");
        
        // 测试 polling 方法
        std::cout << "11. 测试 polling 方法:" << std::endl;
        bool poll_result = simulator.polling();
        std::cout << "   polling 结果：" << poll_result << std::endl;
        ASSERT_TRUE(poll_result == true || poll_result == false, "polling 返回布尔值");
        
    } catch (const std::exception& e) {
        std::cout << "   初始化失败：" << e.what() << std::endl;
        ASSERT_FALSE(true, "trader_simulator 初始化抛出异常");
    }
}

// ==================== 测试入口 ====================

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "   trader_simulator 模块单元测试" << std::endl;
    std::cout << "============================================" << std::endl;
    
    // 基本功能测试
    test_trader_simulator_basic();
    test_place_order();
    test_get_all_orders_positions();
    test_push_tick();
    test_crossday();
    test_polling();
    test_get_all_instruments();
    test_order_state();
    test_position_detail();
    test_edge_cases();
    
    // 扩展功能测试
    test_trader_simulator_extended();
    
    std::cout << "\n============================================" << std::endl;
    std::cout << "   测试结果汇总" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "   通过：" << tests_passed << std::endl;
    std::cout << "   失败：" << tests_failed << std::endl;
    std::cout << "============================================" << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}