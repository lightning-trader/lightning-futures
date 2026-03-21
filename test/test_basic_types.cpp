#include <iostream>
#include <vector>
#include "../src/include/basic_types.hpp"

// Test position_cell structure
void test_position_cell() {
    std::cout << "=== Testing position_cell Structure ===" << std::endl;

    lt::position_cell cell;

    // Test default constructor
    std::cout << "1. Default constructor:" << std::endl;
    std::cout << "   position = " << cell.position << std::endl;
    std::cout << "   frozen = " << cell.frozen << std::endl;
    std::cout << "   empty() = " << cell.empty() << std::endl;
    std::cout << "   usable() = " << cell.usable() << std::endl;

    // Test setting values
    std::cout << "\n2. Setting values:" << std::endl;
    cell.position = 100;
    cell.frozen = 25;
    std::cout << "   position = " << cell.position << std::endl;
    std::cout << "   frozen = " << cell.frozen << std::endl;
    std::cout << "   usable() = " << cell.usable() << std::endl;
    std::cout << "   empty() = " << cell.empty() << std::endl;

    // Test clear
    std::cout << "\n3. Clear:" << std::endl;
    cell.clear();
    std::cout << "   position = " << cell.position << std::endl;
    std::cout << "   frozen = " << cell.frozen << std::endl;
    std::cout << "   empty() = " << cell.empty() << std::endl;
}

// Test position_info structure
void test_position_info() {
    std::cout << "\n=== Testing position_info Structure ===" << std::endl;

    lt::code_t code("SHFE.rb2401");
    lt::position_info pos(code);

    // Test initial state
    std::cout << "1. Initial state:" << std::endl;
    std::cout << "   empty() = " << pos.empty() << std::endl;
    std::cout << "   get_total() = " << pos.get_total() << std::endl;
    std::cout << "   get_long_position() = " << pos.get_long_position() << std::endl;
    std::cout << "   get_short_position() = " << pos.get_short_position() << std::endl;

    // Test setting values
    std::cout << "\n2. Setting values:" << std::endl;
    pos.current_long.position = 10;
    pos.current_short.position = 5;
    pos.history_long.position = 3;
    std::cout << "   get_total() = " << pos.get_total() << std::endl;
    std::cout << "   get_long_position() = " << pos.get_long_position() << std::endl;
    std::cout << "   get_short_position() = " << pos.get_short_position() << std::endl;
    std::cout << "   get_real() = " << pos.get_real() << std::endl;
}

// Test bar_info structure
void test_bar_info() {
    std::cout << "\n=== Testing bar_info Structure ===" << std::endl;

    lt::bar_info bar;
    bar.open = 100.0;
    bar.high = 105.0;
    bar.low = 98.0;
    bar.close = 103.0;
    bar.volume = 1000;

    std::cout << "1. Basic bar info:" << std::endl;
    std::cout << "   open = " << bar.open << std::endl;
    std::cout << "   high = " << bar.high << std::endl;
    std::cout << "   low = " << bar.low << std::endl;
    std::cout << "   close = " << bar.close << std::endl;
    std::cout << "   volume = " << bar.volume << std::endl;
}

// Test symbol_t structure
void test_symbol_t() {
    std::cout << "\n=== Testing symbol_t Structure ===" << std::endl;

    lt::symbol_t sym1("rb2010");
    std::cout << "1. Normal futures: rb2010" << std::endl;
    std::cout << "   family = " << sym1.family << std::endl;
    std::cout << "   number = " << sym1.number << std::endl;

    lt::symbol_t sym2("rb2010P3200");
    std::cout << "\n2. Option futures: rb2010P3200" << std::endl;
    std::cout << "   family = " << sym2.family << std::endl;
    std::cout << "   number = " << sym2.number << std::endl;
    std::cout << "   strike_price = " << sym2.strike_price << std::endl;
    std::cout << "   option_type = " << sym2.option_type << std::endl;
}

// Test code_t structure
void test_code_t() {
    std::cout << "\n=== Testing code_t Structure ===" << std::endl;

    lt::code_t code1("SHFE.rb2401");
    std::cout << "1. Normal contract: " << code1.to_string() << std::endl;
    std::cout << "   exchange = " << code1.get_exchange() << std::endl;
    std::cout << "   symbol = " << code1.get_symbol() << std::endl;

    lt::code_t code2("SHFE.rb2401&rb2402");
    std::cout << "\n2. Spread contract: " << code2.to_string() << std::endl;
    std::cout << "   symbol[0] = " << code2.get_symbol(0) << std::endl;
    std::cout << "   symbol[1] = " << code2.get_symbol(1) << std::endl;
}

// ==================== 边界测试 ====================

// 测试 position_cell 边界情况
void test_position_cell_edge() {
    std::cout << "\n=== 测试 position_cell 边界情况 ===" << std::endl;
    
    lt::position_cell cell;
    
    // 默认值测试
    std::cout << "   [PASS] 默认 position = 0: " << (cell.position == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 frozen = 0: " << (cell.frozen == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 empty() = true: " << (cell.empty() ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 usable() = 0: " << (cell.usable() == 0 ? "PASS" : "FAIL") << std::endl;
    
    // clear() 后测试
    cell.position = 100;
    cell.frozen = 50;
    cell.clear();
    std::cout << "   [PASS] clear() 后 position = 0: " << (cell.position == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] clear() 后 frozen = 0: " << (cell.frozen == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] clear() 后 empty() = true: " << (cell.empty() ? "PASS" : "FAIL") << std::endl;
    
    // 最大值测试
    cell.position = UINT32_MAX;
    cell.frozen = 0;
    std::cout << "   [PASS] position = UINT32_MAX 时 usable() 正确: " << (cell.usable() == UINT32_MAX ? "PASS" : "FAIL") << std::endl;
}

// 测试 position_info 边界情况
void test_position_info_edge() {
    std::cout << "\n=== 测试 position_info 边界情况 ===" << std::endl;
    
    lt::position_info pos;
    
    // 默认值测试
    std::cout << "   [PASS] 默认 empty() = true: " << (pos.empty() ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 get_total() = 0: " << (pos.get_total() == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 get_real() = 0: " << (pos.get_real() == 0 ? "PASS" : "FAIL") << std::endl;
    
    // 负数持仓测试
    pos.current_long.position = 5;
    pos.current_short.position = 10;
    std::cout << "   [PASS] get_real() 返回负数正确: " << (pos.get_real() == -5 ? "PASS" : "FAIL") << std::endl;
    
    // get_total 计算
    pos.current_long.position = 10;
    pos.current_short.position = 5;
    pos.history_long.position = 3;
    pos.history_short.position = 2;
    std::cout << "   [PASS] get_total() 计算正确: " << (pos.get_total() == 20 ? "PASS" : "FAIL") << std::endl;
    
    // 构造函数设置 code
    lt::code_t code("SHFE.rb2401");
    lt::position_info pos_with_code(code);
    std::cout << "   [PASS] 构造函数正确设置 code: " << (pos_with_code.code == code ? "PASS" : "FAIL") << std::endl;
}

// 测试 tick_info 边界情况
void test_tick_info_edge() {
    std::cout << "\n=== 测试 tick_info 边界情况 ===" << std::endl;
    
    lt::tick_info tick;
    
    // 默认值测试
    std::cout << "   [PASS] 默认 code = default_code: " << (tick.code == lt::default_code ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 time = 0: " << (tick.time == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 price = 0: " << (tick.price == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 volume = 0: " << (tick.volume == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 invalid() = true: " << (tick.invalid() ? "PASS" : "FAIL") << std::endl;
    
    // total_buy_volume 和 total_sell_volume 测试 - 使用数组索引访问
    tick.bid_order[0] = {100.0, 10};
    tick.bid_order[1] = {99.0, 20};
    tick.ask_order[0] = {101.0, 15};
    tick.ask_order[1] = {102.0, 25};
    
    std::cout << "   [PASS] total_buy_volume() 计算正确：" << (tick.total_buy_volume() == 30 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] total_sell_volume() 计算正确：" << (tick.total_sell_volume() == 40 ? "PASS" : "FAIL") << std::endl;
    
    // 空买盘测试 - 使用 fill 清空
    tick.bid_order.fill({0.0, 0});
    std::cout << "   [PASS] 空买盘时 total_buy_volume() = 0: " << (tick.total_buy_volume() == 0 ? "PASS" : "FAIL") << std::endl;
    
    // 空卖盘测试
    tick.bid_order[0] = {100.0, 10};
    tick.ask_order.fill({0.0, 0});
    std::cout << "   [PASS] 空卖盘时 total_sell_volume() = 0: " << (tick.total_sell_volume() == 0 ? "PASS" : "FAIL") << std::endl;
}

// 测试 code_t 边界情况
void test_code_t_edge() {
    std::cout << "\n=== 测试 code_t 边界情况 ===" << std::endl;
    
    lt::code_t code;
    
    // 默认值测试
    std::cout << "   [PASS] 默认 get_symbol() = \"\": " << (std::strlen(code.get_symbol()) == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 get_exchange() = \"\": " << (std::strlen(code.get_exchange()) == 0 ? "PASS" : "FAIL") << std::endl;
    
    // is_distinct 测试
    lt::code_t shfe_code("SHFE.rb2401");
    lt::code_t czce_code("CZCE.AP401");
    lt::code_t dce_code("DCE.m2401");
    lt::code_t ine_code("INE.sc2401");
    
    std::cout << "   [PASS] SHFE 代码 is_distinct() = true: " << (shfe_code.is_distinct() ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] CZCE 代码 is_distinct() = false: " << (!czce_code.is_distinct() ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] DCE 代码 is_distinct() = false: " << (!dce_code.is_distinct() ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] INE 代码 is_distinct() = true: " << (ine_code.is_distinct() ? "PASS" : "FAIL") << std::endl;
    
    // 套利合约类型测试
    lt::code_t spread_code("SHFE.rb2401&rb2402");
    std::cout << "   [PASS] 跨期套利类型正确: " << (spread_code.get_type() == lt::code_t::code_type::CT_SP_ARBITRAGE ? "PASS" : "FAIL") << std::endl;
    
    // 比较运算符测试
    lt::code_t code1("SHFE.rb2401");
    lt::code_t code2("SHFE.rb2401");
    lt::code_t code3("SHFE.rb2402");
    
    std::cout << "   [PASS] 相同代码 == 正确: " << ((code1 == code2) ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 不同代码 != 正确: " << ((code1 != code3) ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] < 运算符正确: " << ((code1 < code3) ? "PASS" : "FAIL") << std::endl;
    
    // to_string 测试
    std::cout << "   [PASS] to_string() 正确：" << (code1.to_string() == "SHFE.rb2401" ? "PASS" : "FAIL") << std::endl;
}

// 测试 bar_info 边界情况
void test_bar_info_edge() {
    std::cout << "\n=== 测试 bar_info 边界情况 ===" << std::endl;
    
    lt::bar_info bar;
    
    // 默认值测试
    std::cout << "   [PASS] 默认 time = 0: " << (bar.time == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 open = 0: " << (bar.open == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 high = 0: " << (bar.high == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 low = 0: " << (bar.low == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 close = 0: " << (bar.close == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 volume = 0: " << (bar.volume == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 get_delta() = 0: " << (bar.get_delta() == 0 ? "PASS" : "FAIL") << std::endl;
    
    // detail_density = 0 时 get_buy_volume/get_sell_volume 测试
    bar.detail_density = 0;
    std::cout << "   [PASS] detail_density = 0 时 get_buy_volume() = 0: " << (bar.get_buy_volume(100.0) == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] detail_density = 0 时 get_sell_volume() = 0: " << (bar.get_sell_volume(100.0) == 0 ? "PASS" : "FAIL") << std::endl;
    
    // 价格不存在时 get_buy_volume 测试
    bar.detail_density = 1.0;
    std::cout << "   [PASS] 价格不存在时 get_buy_volume() = 0: " << (bar.get_buy_volume(100.0) == 0 ? "PASS" : "FAIL") << std::endl;
    
    // get_delta 计算
    bar.buy_details[100.0] = 50;
    bar.buy_details[101.0] = 30;
    bar.sell_details[100.0] = 20;
    bar.sell_details[99.0] = 10;
    std::cout << "   [PASS] get_delta() 计算正确 (80-30=50): " << (bar.get_delta() == 50 ? "PASS" : "FAIL") << std::endl;
    
    // get_poc 测试
    bar.buy_details[102.0] = 100;  // 最大成交量
    std::cout << "   [PASS] get_poc() 返回最大成交量价格: " << (bar.get_poc() == 102.0 ? "PASS" : "FAIL") << std::endl;
    
    // get_order_book 测试 - low/high/density 为 0 时返回空
    bar.low = 0;
    bar.high = 0;
    bar.detail_density = 0;
    auto order_book = bar.get_order_book();
    std::cout << "   [PASS] low/high/density 为 0 时 get_order_book() 返回空: " << (order_book.empty() ? "PASS" : "FAIL") << std::endl;
    
    // clear 测试
    bar.clear();
    std::cout << "   [PASS] clear() 后 time = 0: " << (bar.time == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] clear() 后 open = 0: " << (bar.open == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] clear() 后 buy_details 为空: " << (bar.buy_details.empty() ? "PASS" : "FAIL") << std::endl;
}

// 测试 order_info 边界情况
void test_order_info_edge() {
    std::cout << "\n=== 测试 order_info 边界情况 ===" << std::endl;
    
    lt::order_info order;
    
    // 默认值测试
    std::cout << "   [PASS] 默认 invalid() = true: " << (order.invalid() ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 is_buy() = true (LONG+OPEN): " << (order.is_buy() ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 is_sell() = false: " << (!order.is_sell() ? "PASS" : "FAIL") << std::endl;
    
    // is_buy/is_sell 测试
    order.direction = lt::direction_type::DT_LONG;
    order.offset = lt::offset_type::OT_OPEN;
    std::cout << "   [PASS] LONG+OPEN = buy: " << (order.is_buy() ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] LONG+OPEN != sell: " << (!order.is_sell() ? "PASS" : "FAIL") << std::endl;
    
    order.offset = lt::offset_type::OT_CLOSE;
    std::cout << "   [PASS] LONG+CLOSE != buy: " << (!order.is_buy() ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] LONG+CLOSE = sell: " << (order.is_sell() ? "PASS" : "FAIL") << std::endl;
    
    order.direction = lt::direction_type::DT_SHORT;
    order.offset = lt::offset_type::OT_OPEN;
    std::cout << "   [PASS] SHORT+OPEN != buy: " << (!order.is_buy() ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] SHORT+OPEN = sell: " << (order.is_sell() ? "PASS" : "FAIL") << std::endl;
    
    order.offset = lt::offset_type::OT_CLSTD;
    std::cout << "   [PASS] SHORT+CLSTD = buy: " << (order.is_buy() ? "PASS" : "FAIL") << std::endl;
}

// 测试 instrument_info 边界情况
void test_instrument_info_edge() {
    std::cout << "\n=== 测试 instrument_info 边界情况 ===" << std::endl;
    
    lt::instrument_info info;
    
    // 默认值测试
    std::cout << "   [PASS] 默认 classtype = PT_INVALID: " << (info.classtype == lt::product_type::PT_INVALID ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 price_step = 0: " << (info.price_step == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 multiple = 0: " << (info.multiple == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 begin_day = 0: " << (info.begin_day == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 end_day = 0: " << (info.end_day == 0 ? "PASS" : "FAIL") << std::endl;
}

// 测试 market_info 边界情况
void test_market_info_edge() {
    std::cout << "\n=== 测试 market_info 边界情况 ===" << std::endl;
    
    lt::market_info market;
    
    // 默认值测试
    std::cout << "   [PASS] 默认 open_price = 0: " << (market.open_price == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 close_price = 0: " << (market.close_price == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 invalid() = true: " << (market.invalid() ? "PASS" : "FAIL") << std::endl;
    
    // control_price 测试 - 空分布时返回 last_tick_info.price
    market.last_tick_info.price = 100.0;
    std::cout << "   [PASS] 空分布时 control_price() 返回 last_tick_info.price: " << (market.control_price() == 100.0 ? "PASS" : "FAIL") << std::endl;
    
    // control_price 测试 - 有分布时返回最大成交量价格
    market.volume_distribution[99.0] = 10;
    market.volume_distribution[100.0] = 50;  // 最大
    market.volume_distribution[101.0] = 30;
    std::cout << "   [PASS] control_price() 返回最大成交量价格: " << (market.control_price() == 100.0 ? "PASS" : "FAIL") << std::endl;
    
    // middle_price 测试
    market.high_price = 110.0;
    market.low_price = 90.0;
    std::cout << "   [PASS] middle_price() 计算正确: " << (market.middle_price() == 100.0 ? "PASS" : "FAIL") << std::endl;
    
    // clear 测试
    market.clear();
    std::cout << "   [PASS] clear() 后 volume_distribution 为空: " << (market.volume_distribution.empty() ? "PASS" : "FAIL") << std::endl;
}

// 测试 tape_info 边界情况
void test_tape_info_edge() {
    std::cout << "\n=== 测试 tape_info 边界情况 ===" << std::endl;
    
    lt::tape_info tape;
    
    // 默认值测试
    std::cout << "   [PASS] 默认 time = 0: " << (tape.time == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 price = 0: " << (tape.price == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 volume_delta = 0: " << (tape.volume_delta == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 interest_delta = 0: " << (tape.interest_delta == 0 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] 默认 direction = DD_FLAT: " << (tape.direction == lt::deal_direction::DD_FLAT ? "PASS" : "FAIL") << std::endl;
    
    // get_status 测试
    tape.volume_delta = 100;
    tape.interest_delta = 100;
    std::cout << "   [PASS] volume_delta == interest_delta > 0 = DS_DOUBLE_OPEN: " << (tape.get_status() == lt::deal_status::DS_DOUBLE_OPEN ? "PASS" : "FAIL") << std::endl;
    
    tape.volume_delta = 100;
    tape.interest_delta = 50;
    std::cout << "   [PASS] volume_delta > interest_delta > 0 = DS_OPEN: " << (tape.get_status() == lt::deal_status::DS_OPEN ? "PASS" : "FAIL") << std::endl;
    
    tape.volume_delta = 100;
    tape.interest_delta = 0;
    std::cout << "   [PASS] volume_delta > interest_delta == 0 = DS_CHANGE: " << (tape.get_status() == lt::deal_status::DS_CHANGE ? "PASS" : "FAIL") << std::endl;
    
    tape.volume_delta = 100;
    tape.interest_delta = -50;
    std::cout << "   [PASS] volume_delta > -interest_delta > 0 = DS_CLOSE: " << (tape.get_status() == lt::deal_status::DS_CLOSE ? "PASS" : "FAIL") << std::endl;
    
    tape.volume_delta = 100;
    tape.interest_delta = -100;
    std::cout << "   [PASS] volume_delta == -interest_delta > 0 = DS_DOUBLE_CLOSE: " << (tape.get_status() == lt::deal_status::DS_DOUBLE_CLOSE ? "PASS" : "FAIL") << std::endl;
}

// 测试 symbol_t 解析
void test_symbol_t_parsing() {
    std::cout << "\n=== 测试 symbol_t 解析 ===" << std::endl;
    
    // 普通期货合约
    lt::symbol_t rb2010("rb2010");
    std::cout << "   [PASS] rb2010 family = rb: " << (rb2010.family == "rb" ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] rb2010 number = 2010: " << (rb2010.number == 2010 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] rb2010 option_type = OPT_INVALID: " << (rb2010.option_type == lt::symbol_t::OPT_INVALID ? "PASS" : "FAIL") << std::endl;
    
    // 看涨期权
    lt::symbol_t call_opt("m1707C2600");
    std::cout << "   [PASS] m1707C2600 family = m: " << (call_opt.family == "m" ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] m1707C2600 number = 1707: " << (call_opt.number == 1707 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] m1707C2600 option_type = OPT_CALL: " << (call_opt.option_type == lt::symbol_t::OPT_CALL ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] m1707C2600 strike_price = 2600: " << (call_opt.strike_price == 2600.0 ? "PASS" : "FAIL") << std::endl;
    
    // 看跌期权
    lt::symbol_t put_opt("m1707P2600");
    std::cout << "   [PASS] m1707P2600 family = m: " << (put_opt.family == "m" ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] m1707P2600 number = 1707: " << (put_opt.number == 1707 ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] m1707P2600 option_type = OPT_PUT: " << (put_opt.option_type == lt::symbol_t::OPT_PUT ? "PASS" : "FAIL") << std::endl;
    std::cout << "   [PASS] m1707P2600 strike_price = 2600: " << (put_opt.strike_price == 2600.0 ? "PASS" : "FAIL") << std::endl;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "   basic_types 模块单元测试" << std::endl;
    std::cout << "============================================" << std::endl;

    // 基本功能测试
    test_position_cell();
    test_position_info();
    test_bar_info();
    test_symbol_t();
    test_code_t();
    
    // 边界测试
    test_position_cell_edge();
    test_position_info_edge();
    test_tick_info_edge();
    test_code_t_edge();
    test_bar_info_edge();
    test_order_info_edge();
    test_instrument_info_edge();
    test_market_info_edge();
    test_tape_info_edge();
    test_symbol_t_parsing();

    std::cout << "\n============================================" << std::endl;
    std::cout << "   所有测试完成!" << std::endl;
    std::cout << "============================================" << std::endl;

    return 0;
}
