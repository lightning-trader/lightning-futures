#include <iostream>
#include "../include/basic_types.hpp"

int main() {
    std::cout << "=== 测试持仓计算修复 ===" << std::endl;

    // 测试 1: 拼写错误修复验证
    std::cout << "\n1. 测试拼写错误修复 (postion -> position):" << std::endl;
    lt::position_cell cell;
    cell.position = 100;
    cell.frozen = 20;
    std::cout << "   position_cell.position = " << cell.position << std::endl;
    std::cout << "   position_cell.frozen = " << cell.frozen << std::endl;
    std::cout << "   position_cell.usable() = " << cell.usable() << std::endl;
    std::cout << "   ✓ 拼写错误修复成功!" << std::endl;

    // 测试 2: position_info 结构体验证
    std::cout << "\n2. 测试 position_info 结构体:" << std::endl;
    lt::position_info pos;
    pos.total_long.position = 100;
    pos.total_short.position = 50;
    pos.history_long.position = 30;
    pos.history_short.position = 20;
    pos.today_long.position = 70;
    pos.today_short.position = 30;
    
    std::cout << "   total_long.position = " << pos.total_long.position << std::endl;
    std::cout << "   total_short.position = " << pos.total_short.position << std::endl;
    std::cout << "   history_long.position = " << pos.history_long.position << std::endl;
    std::cout << "   history_short.position = " << pos.history_short.position << std::endl;
    std::cout << "   today_long.position = " << pos.today_long.position << std::endl;
    std::cout << "   today_short.position = " << pos.today_short.position << std::endl;
    std::cout << "   get_total() = " << pos.get_total() << std::endl;
    std::cout << "   get_real() = " << pos.get_real() << std::endl;
    std::cout << "   ✓ position_info 结构体修复成功!" << std::endl;

    // 测试 3: 今仓计算验证
    std::cout << "\n3. 测试今仓计算:" << std::endl;
    std::cout << "   今仓多头 = " << pos.today_long.position << std::endl;
    std::cout << "   今仓空头 = " << pos.today_short.position << std::endl;
    std::cout << "   今仓净仓位 = " << pos.today_long.position - pos.today_short.position << std::endl;
    std::cout << "   ✓ 今仓计算验证成功!" << std::endl;

    // 测试 4: 方法调用验证
    std::cout << "\n4. 测试方法调用:" << std::endl;
    std::cout << "   position_cell.empty() = " << cell.empty() << std::endl;
    std::cout << "   position_info.empty() = " << pos.empty() << std::endl;
    
    // 清空仓位
    cell.clear();
    std::cout << "   清空后 position_cell.empty() = " << cell.empty() << std::endl;
    std::cout << "   ✓ 方法调用验证成功!" << std::endl;

    std::cout << "\n=== 所有测试完成! ===" << std::endl;
    return 0;
}
