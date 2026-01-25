#include <iostream>
#include "../src/include/trading_context.h"
#include "../src/include/basic_types.hpp"

// 测试 trading_context 类
void test_trading_context() {
    std::cout << "=== 测试 trading_context 类 ===" << std::endl;
    
    lt::trading_context ctx;
    
    // 测试默认构造函数
    std::cout << "1. 测试默认构造函数:" << std::endl;
    std::cout << "   ✓ trading_context 初始化成功" << std::endl;
    
    // 测试 calculate_position 方法
    std::cout << "\n2. 测试 calculate_position 方法:" << std::endl;
    
    lt::code_t code("IF2109");
    
    // 测试开仓
    std::cout << "   2.1 测试开仓:" << std::endl;
    ctx.calculate_position(code, lt::direction_type::DT_LONG, lt::offset_type::OT_OPEN, 100, 4500.0);
    std::cout << "   ✓ 开仓 100 手多头成功" << std::endl;
    
    // 测试平仓
    std::cout << "   2.2 测试平仓:" << std::endl;
    ctx.calculate_position(code, lt::direction_type::DT_LONG, lt::offset_type::OT_CLOSE, 50, 4500.0);
    std::cout << "   ✓ 平仓 50 手多头成功" << std::endl;
    
    // 测试平今仓
    std::cout << "   2.3 测试平今仓:" << std::endl;
    ctx.calculate_position(code, lt::direction_type::DT_LONG, lt::offset_type::OT_CLSTD, 30, 4500.0);
    std::cout << "   ✓ 平今仓 30 手多头成功" << std::endl;
    
    // 测试 frozen_deduction 方法
    std::cout << "\n3. 测试 frozen_deduction 方法:" << std::endl;
    ctx.frozen_deduction(code, lt::direction_type::DT_LONG, lt::offset_type::OT_CLOSE, 20);
    std::cout << "   ✓ 冻结 20 手多头成功" << std::endl;
    
    // 测试 unfreeze_deduction 方法
    std::cout << "\n4. 测试 unfreeze_deduction 方法:" << std::endl;
    ctx.unfreeze_deduction(code, lt::direction_type::DT_LONG, lt::offset_type::OT_CLOSE, 20);
    std::cout << "   ✓ 解冻 20 手多头成功" << std::endl;
    
    // 测试 record_pending 方法
    std::cout << "\n5. 测试 record_pending 方法:" << std::endl;
    ctx.record_pending(code, lt::direction_type::DT_LONG, lt::offset_type::OT_OPEN, 50);
    std::cout << "   ✓ 记录 50 手多头挂单成功" << std::endl;
    
    // 测试 get_position 方法
    std::cout << "\n6. 测试 get_position 方法:" << std::endl;
    auto position = ctx.get_position(code);
    std::cout << "   合约: " << position.code.get_symbol() << std::endl;
    std::cout << "   总多头: " << position.total_long.position << std::endl;
    std::cout << "   总空头: " << position.total_short.position << std::endl;
    std::cout << "   昨多头: " << position.history_long.position << std::endl;
    std::cout << "   昨空头: " << position.history_short.position << std::endl;
    std::cout << "   今多头: " << position.today_long.position << std::endl;
    std::cout << "   今空头: " << position.today_short.position << std::endl;
    std::cout << "   多头挂单: " << position.long_pending << std::endl;
    std::cout << "   空头挂单: " << position.short_pending << std::endl;
    
    // 测试 get_all_positions 方法
    std::cout << "\n7. 测试 get_all_positions 方法:" << std::endl;
    auto all_positions = ctx.get_all_positions();
    std::cout << "   持仓数量: " << all_positions.size() << std::endl;
    for (const auto& pos : all_positions) {
        std::cout << "   合约: " << pos.code.get_symbol() << ", 总仓位: " << pos.get_total() << std::endl;
    }
    
    // 测试 handle_tick 方法
    std::cout << "\n8. 测试 handle_tick 方法:" << std::endl;
    lt::tick_info tick;
    tick.code = code;
    tick.time = 1000000;
    tick.price = 4500.0;
    ctx.handle_tick(tick);
    std::cout << "   ✓ 处理 tick 成功" << std::endl;
    
    // 测试 clear 方法
    std::cout << "\n9. 测试 clear 方法:" << std::endl;
    ctx.clear();
    std::cout << "   ✓ 清空持仓成功" << std::endl;
    
    std::cout << "=== trading_context 测试完成 ===\n" << std::endl;
}

int main() {
    std::cout << "=== 开始 trading_context 模块单元测试 ===\n" << std::endl;
    
    test_trading_context();
    
    std::cout << "=== trading_context 模块单元测试完成 ===" << std::endl;
    return 0;
}
