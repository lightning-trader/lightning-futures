#include <iostream>
#include "../src/simulator/trader_simulator.h"
#include "../src/include/params.hpp"
#include "../src/include/basic_types.hpp"

// 测试 trader_simulator 类
void test_trader_simulator() {
    std::cout << "=== 测试 trader_simulator 类 ===" << std::endl;
    
    // 测试初始化
    std::cout << "1. 测试初始化:" << std::endl;
    
    lt::params config;
    config.set("interval", 100);
    config.set("initial_capital", 1000000.0);
    config.set("contract_config", "../bin/config/contract.csv");
    
    try {
        lt::driver::trader_simulator simulator(config);
        std::cout << "   ✓ trader_simulator 初始化成功" << std::endl;
        
        // 测试 get_trading_day 方法
        std::cout << "\n2. 测试 get_trading_day 方法:" << std::endl;
        auto trading_day = simulator.get_trading_day();
        std::cout << "   trading_day = " << trading_day << std::endl;
        
        // 测试 is_usable 方法
        std::cout << "\n3. 测试 is_usable 方法:" << std::endl;
        bool usable = simulator.is_usable();
        std::cout << "   is_usable() = " << usable << std::endl;
        
        // 测试 get_all_instruments 方法
        std::cout << "\n4. 测试 get_all_instruments 方法:" << std::endl;
        auto instruments = simulator.get_all_instruments();
        std::cout << "   合约数量: " << instruments.size() << std::endl;
        
        // 测试 place_order 方法
        std::cout << "\n5. 测试 place_order 方法:" << std::endl;
        
        lt::code_t code("IF2109");
        
        // 测试开仓
        std::cout << "   5.1 测试开仓:" << std::endl;
        auto estid_open = simulator.place_order(lt::offset_type::OT_OPEN, lt::direction_type::DT_LONG, code, 10, 4500.0, lt::order_flag::OF_NOR);
        std::cout << "   ✓ 开仓 10 手多头成功，订单 ID: " << estid_open << std::endl;
        
        // 测试平仓
        std::cout << "   5.2 测试平仓:" << std::endl;
        auto estid_close = simulator.place_order(lt::offset_type::OT_CLOSE, lt::direction_type::DT_LONG, code, 5, 4500.0, lt::order_flag::OF_NOR);
        std::cout << "   ✓ 平仓 5 手多头成功，订单 ID: " << estid_close << std::endl;
        
        // 测试平今仓
        std::cout << "   5.3 测试平今仓:" << std::endl;
        auto estid_clstd = simulator.place_order(lt::offset_type::OT_CLSTD, lt::direction_type::DT_LONG, code, 3, 4500.0, lt::order_flag::OF_NOR);
        std::cout << "   ✓ 平今仓 3 手多头成功，订单 ID: " << estid_clstd << std::endl;
        
        // 测试 cancel_order 方法
        std::cout << "\n6. 测试 cancel_order 方法:" << std::endl;
        bool cancel_result = simulator.cancel_order(estid_open);
        std::cout << "   撤单结果: " << cancel_result << std::endl;
        
        // 测试 get_all_orders 方法
        std::cout << "\n7. 测试 get_all_orders 方法:" << std::endl;
        auto orders = simulator.get_all_orders();
        std::cout << "   订单数量: " << orders.size() << std::endl;
        
        // 测试 get_all_positions 方法
        std::cout << "\n8. 测试 get_all_positions 方法:" << std::endl;
        auto positions = simulator.get_all_positions();
        std::cout << "   持仓数量: " << positions.size() << std::endl;
        for (const auto& pos : positions) {
            std::cout << "   合约: " << pos.code.get_symbol() << std::endl;
            std::cout << "   总多头: " << pos.total_long << std::endl;
            std::cout << "   总空头: " << pos.total_short << std::endl;
            std::cout << "   昨多头: " << pos.history_long << std::endl;
            std::cout << "   昨空头: " << pos.history_short << std::endl;
        }
        
        // 测试 crossday 方法
        std::cout << "\n9. 测试 crossday 方法:" << std::endl;
        simulator.crossday(20210901);
        std::cout << "   ✓ 跨天处理成功，交易日: 20210901" << std::endl;
        
        // 测试 push_tick 方法
        std::cout << "\n10. 测试 push_tick 方法:" << std::endl;
        lt::tick_info tick;
        tick.code = code;
        tick.time = 1000000;
        tick.price = 4500.0;
        tick.volume = 10000;
        tick.amount = 45000000.0;
        
        std::vector<const lt::tick_info*> ticks;
        ticks.push_back(&tick);
        
        simulator.push_tick(ticks);
        std::cout << "   ✓ 推送 tick 成功" << std::endl;
        
        // 测试 poll 方法
        std::cout << "\n11. 测试 poll 方法:" << std::endl;
        bool poll_result = simulator.poll();
        std::cout << "   poll 结果: " << poll_result << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "   ✗ 初始化失败: " << e.what() << std::endl;
    }
    
    std::cout << "=== trader_simulator 测试完成 ===\n" << std::endl;
}

int main() {
    std::cout << "=== 开始 trader_simulator 模块单元测试 ===\n" << std::endl;
    
    test_trader_simulator();
    
    std::cout << "=== trader_simulator 模块单元测试完成 ===" << std::endl;
    return 0;
}
