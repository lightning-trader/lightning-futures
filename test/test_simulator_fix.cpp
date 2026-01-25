#include <iostream>
#include "../simulator/trader_simulator.h"
#include "../include/params.hpp"

int main() {
    std::cout << "=== 测试模拟器持仓计算 ===" << std::endl;

    // 测试 1: 拼写错误检查
    std::cout << "\n1. 测试拼写错误检查:" << std::endl;
    lt::driver::trader_simulator::position_item item;
    item.postion = 100; // 这里应该是 position，但模拟器中使用的是 postion
    item.frozen = 20;
    std::cout << "   position_item.postion = " << item.postion << std::endl;
    std::cout << "   position_item.frozen = " << item.frozen << std::endl;
    std::cout << "   position_item.usable() = " << item.usable() << std::endl;
    std::cout << "   ✓ 拼写错误检查完成!" << std::endl;

    // 测试 2: 模拟器初始化
    std::cout << "\n2. 测试模拟器初始化:" << std::endl;
    lt::params config;
    config.set("interval", 100);
    config.set("initial_capital", 1000000.0);
    config.set("contract_config", "../../bin/config/contract.csv");
    
    try {
        lt::driver::trader_simulator simulator(config);
        std::cout << "   ✓ 模拟器初始化成功!" << std::endl;
        
        // 测试 3: 开仓测试
        std::cout << "\n3. 测试开仓:" << std::endl;
        lt::code_t code("IF2109");
        
        // 模拟开仓 10 手多头
        auto estid = simulator.place_order(lt::offset_type::OT_OPEN, lt::direction_type::DT_LONG, code, 10, 4500.0, lt::order_flag::OF_NOR);
        std::cout << "   开仓 10 手多头，订单 ID: " << estid << std::endl;
        
        // 测试 4: 平仓测试
        std::cout << "\n4. 测试平仓:" << std::endl;
        
        // 模拟平仓 5 手多头
        auto close_estid = simulator.place_order(lt::offset_type::OT_CLOSE, lt::direction_type::DT_LONG, code, 5, 4500.0, lt::order_flag::OF_NOR);
        std::cout << "   平仓 5 手多头，订单 ID: " << close_estid << std::endl;
        
        // 测试 5: 平今仓测试
        std::cout << "\n5. 测试平今仓:" << std::endl;
        
        // 模拟平今仓 3 手多头
        auto clstd_estid = simulator.place_order(lt::offset_type::OT_CLSTD, lt::direction_type::DT_LONG, code, 3, 4500.0, lt::order_flag::OF_NOR);
        std::cout << "   平今仓 3 手多头，订单 ID: " << clstd_estid << std::endl;
        
        // 测试 6: 仓位查询
        std::cout << "\n6. 测试仓位查询:" << std::endl;
        auto positions = simulator.get_all_positions();
        for (const auto& pos : positions) {
            std::cout << "   合约: " << pos.code.get_symbol() << std::endl;
            std::cout << "   总多头: " << pos.total_long << std::endl;
            std::cout << "   总空头: " << pos.total_short << std::endl;
            std::cout << "   昨多头: " << pos.history_long << std::endl;
            std::cout << "   昨空头: " << pos.history_short << std::endl;
        }
        std::cout << "   ✓ 仓位查询完成!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "   ✗ 模拟器初始化失败: " << e.what() << std::endl;
    }

    std::cout << "\n=== 测试完成! ===" << std::endl;
    return 0;
}
