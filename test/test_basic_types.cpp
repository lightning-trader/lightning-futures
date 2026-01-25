#include <iostream>
#include <vector>
#include "../src/include/basic_types.hpp"

// 测试 position_cell 结构体
void test_position_cell() {
    std::cout << "=== 测试 position_cell 结构体 ===" << std::endl;
    
    lt::position_cell cell;
    
    // 测试默认构造函数
    std::cout << "1. 测试默认构造函数:" << std::endl;
    std::cout << "   position = " << cell.position << std::endl;
    std::cout << "   frozen = " << cell.frozen << std::endl;
    std::cout << "   empty() = " << cell.empty() << std::endl;
    std::cout << "   usable() = " << cell.usable() << std::endl;
    
    // 测试设置值
    std::cout << "\n2. 测试设置值:" << std::endl;
    cell.position = 100;
    cell.frozen = 20;
    std::cout << "   position = " << cell.position << std::endl;
    std::cout << "   frozen = " << cell.frozen << std::endl;
    std::cout << "   empty() = " << cell.empty() << std::endl;
    std::cout << "   usable() = " << cell.usable() << std::endl;
    
    // 测试 clear() 方法
    std::cout << "\n3. 测试 clear() 方法:" << std::endl;
    cell.clear();
    std::cout << "   position = " << cell.position << std::endl;
    std::cout << "   frozen = " << cell.frozen << std::endl;
    std::cout << "   empty() = " << cell.empty() << std::endl;
    
    std::cout << "=== position_cell 测试完成 ===\n" << std::endl;
}

// 测试 position_info 结构体
void test_position_info() {
    std::cout << "=== 测试 position_info 结构体 ===" << std::endl;
    
    lt::position_info pos;
    
    // 测试默认构造函数
    std::cout << "1. 测试默认构造函数:" << std::endl;
    std::cout << "   total_long.position = " << pos.total_long.position << std::endl;
    std::cout << "   total_short.position = " << pos.total_short.position << std::endl;
    std::cout << "   history_long.position = " << pos.history_long.position << std::endl;
    std::cout << "   history_short.position = " << pos.history_short.position << std::endl;
    std::cout << "   today_long.position = " << pos.today_long.position << std::endl;
    std::cout << "   today_short.position = " << pos.today_short.position << std::endl;
    std::cout << "   long_pending = " << pos.long_pending << std::endl;
    std::cout << "   short_pending = " << pos.short_pending << std::endl;
    std::cout << "   empty() = " << pos.empty() << std::endl;
    
    // 测试设置值
    std::cout << "\n2. 测试设置值:" << std::endl;
    pos.total_long.position = 100;
    pos.total_short.position = 50;
    pos.history_long.position = 30;
    pos.history_short.position = 20;
    pos.today_long.position = 70;
    pos.today_short.position = 30;
    pos.long_pending = 10;
    pos.short_pending = 5;
    
    std::cout << "   total_long.position = " << pos.total_long.position << std::endl;
    std::cout << "   total_short.position = " << pos.total_short.position << std::endl;
    std::cout << "   history_long.position = " << pos.history_long.position << std::endl;
    std::cout << "   history_short.position = " << pos.history_short.position << std::endl;
    std::cout << "   today_long.position = " << pos.today_long.position << std::endl;
    std::cout << "   today_short.position = " << pos.today_short.position << std::endl;
    std::cout << "   long_pending = " << pos.long_pending << std::endl;
    std::cout << "   short_pending = " << pos.short_pending << std::endl;
    std::cout << "   empty() = " << pos.empty() << std::endl;
    std::cout << "   get_total() = " << pos.get_total() << std::endl;
    std::cout << "   get_real() = " << pos.get_real() << std::endl;
    
    // 测试带参数的构造函数
    std::cout << "\n3. 测试带参数的构造函数:" << std::endl;
    lt::code_t code("IF2109");
    lt::position_info pos_with_code(code);
    std::cout << "   code = " << pos_with_code.code.get_symbol() << std::endl;
    std::cout << "   long_pending = " << pos_with_code.long_pending << std::endl;
    std::cout << "   short_pending = " << pos_with_code.short_pending << std::endl;
    
    std::cout << "=== position_info 测试完成 ===\n" << std::endl;
}

// 测试 bar_info 结构体
void test_bar_info() {
    std::cout << "=== 测试 bar_info 结构体 ===" << std::endl;
    
    lt::bar_info bar;
    
    // 测试默认构造函数
    std::cout << "1. 测试默认构造函数:" << std::endl;
    std::cout << "   time = " << bar.time << std::endl;
    std::cout << "   open = " << bar.open << std::endl;
    std::cout << "   high = " << bar.high << std::endl;
    std::cout << "   low = " << bar.low << std::endl;
    std::cout << "   close = " << bar.close << std::endl;
    std::cout << "   volume = " << bar.volume << std::endl;
    std::cout << "   amount = " << bar.amount << std::endl;
    std::cout << "   get_delta() = " << bar.get_delta() << std::endl;
    
    // 测试设置值
    std::cout << "\n2. 测试设置值:" << std::endl;
    bar.time = 1000000;
    bar.open = 4500.0;
    bar.high = 4550.0;
    bar.low = 4450.0;
    bar.close = 4520.0;
    bar.volume = 10000;
    bar.amount = 45200000.0;
    
    // 测试 buy_details 和 sell_details
    bar.buy_details[4520.0] = 100;
    bar.buy_details[4510.0] = 200;
    bar.sell_details[4530.0] = 150;
    bar.sell_details[4540.0] = 50;
    
    std::cout << "   time = " << bar.time << std::endl;
    std::cout << "   open = " << bar.open << std::endl;
    std::cout << "   high = " << bar.high << std::endl;
    std::cout << "   low = " << bar.low << std::endl;
    std::cout << "   close = " << bar.close << std::endl;
    std::cout << "   volume = " << bar.volume << std::endl;
    std::cout << "   amount = " << bar.amount << std::endl;
    std::cout << "   get_delta() = " << bar.get_delta() << std::endl;
    
    std::cout << "=== bar_info 测试完成 ===\n" << std::endl;
}

// 测试 tick_info 结构体
void test_tick_info() {
    std::cout << "=== 测试 tick_info 结构体 ===" << std::endl;
    
    lt::tick_info tick;
    
    // 测试默认构造函数
    std::cout << "1. 测试默认构造函数:" << std::endl;
    std::cout << "   code = " << tick.code.get_symbol() << std::endl;
    std::cout << "   time = " << tick.time << std::endl;
    std::cout << "   price = " << tick.price << std::endl;
    std::cout << "   volume = " << tick.volume << std::endl;
    std::cout << "   amount = " << tick.amount << std::endl;
    std::cout << "   bid_price[0] = " << tick.bid_price[0] << std::endl;
    std::cout << "   ask_price[0] = " << tick.ask_price[0] << std::endl;
    std::cout << "   bid_volume[0] = " << tick.bid_volume[0] << std::endl;
    std::cout << "   ask_volume[0] = " << tick.ask_volume[0] << std::endl;
    
    // 测试设置值
    std::cout << "\n2. 测试设置值:" << std::endl;
    tick.code = lt::code_t("IF2109");
    tick.time = 1000000;
    tick.price = 4500.0;
    tick.volume = 10000;
    tick.amount = 45000000.0;
    
    // 设置买卖盘口
    for (int i = 0; i < 5; i++) {
        tick.bid_price[i] = 4500.0 - i * 10.0;
        tick.ask_price[i] = 4500.0 + i * 10.0;
        tick.bid_volume[i] = 100 + i * 20;
        tick.ask_volume[i] = 100 + i * 20;
    }
    
    // 设置买卖订单详情
    tick.bid_order.push_back(std::make_pair(4500.0, 100));
    tick.bid_order.push_back(std::make_pair(4490.0, 200));
    tick.ask_order.push_back(std::make_pair(4510.0, 150));
    tick.ask_order.push_back(std::make_pair(4520.0, 50));
    
    std::cout << "   code = " << tick.code.get_symbol() << std::endl;
    std::cout << "   time = " << tick.time << std::endl;
    std::cout << "   price = " << tick.price << std::endl;
    std::cout << "   volume = " << tick.volume << std::endl;
    std::cout << "   amount = " << tick.amount << std::endl;
    std::cout << "   bid_price[0] = " << tick.bid_price[0] << std::endl;
    std::cout << "   ask_price[0] = " << tick.ask_price[0] << std::endl;
    std::cout << "   bid_volume[0] = " << tick.bid_volume[0] << std::endl;
    std::cout << "   ask_volume[0] = " << tick.ask_volume[0] << std::endl;
    
    // 测试 buy_price() 和 sell_price() 方法
    std::cout << "\n3. 测试 buy_price() 和 sell_price() 方法:" << std::endl;
    std::cout << "   buy_price() = " << tick.buy_price() << std::endl;
    std::cout << "   sell_price() = " << tick.sell_price() << std::endl;
    
    std::cout << "=== tick_info 测试完成 ===\n" << std::endl;
}

// 测试 code_t 类
void test_code_t() {
    std::cout << "=== 测试 code_t 类 ===" << std::endl;
    
    // 测试默认构造函数
    std::cout << "1. 测试默认构造函数:" << std::endl;
    lt::code_t code1;
    std::cout << "   get_symbol() = " << code1.get_symbol() << std::endl;
    std::cout << "   is_distinct() = " << code1.is_distinct() << std::endl;
    
    // 测试带参数的构造函数
    std::cout << "\n2. 测试带参数的构造函数:" << std::endl;
    lt::code_t code2("IF2109");
    std::cout << "   get_symbol() = " << code2.get_symbol() << std::endl;
    std::cout << "   is_distinct() = " << code2.is_distinct() << std::endl;
    
    // 测试赋值运算符
    std::cout << "\n3. 测试赋值运算符:" << std::endl;
    lt::code_t code3;
    code3 = code2;
    std::cout << "   get_symbol() = " << code3.get_symbol() << std::endl;
    
    // 测试比较运算符
    std::cout << "\n4. 测试比较运算符:" << std::endl;
    std::cout << "   code2 == code3 = " << (code2 == code3) << std::endl;
    std::cout << "   code1 == code2 = " << (code1 == code2) << std::endl;
    
    std::cout << "=== code_t 测试完成 ===\n" << std::endl;
}

int main() {
    std::cout << "=== 开始 basic_types 模块单元测试 ===\n" << std::endl;
    
    test_position_cell();
    test_position_info();
    test_bar_info();
    test_tick_info();
    test_code_t();
    
    std::cout << "=== basic_types 模块单元测试完成 ===" << std::endl;
    return 0;
}
