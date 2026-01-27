#include <iostream>
#include "../src/include/basic_utils.hpp"

// 测试 basic_utils 模块
void test_basic_utils() {
    std::cout << "=== 测试 basic_utils 模块 ===" << std::endl;
    
    // 测试 make_code 函数
    std::cout << "1. 测试 make_code 函数:" << std::endl;
    
    // 测试字符串参数
    lt::code_t code1 = lt::make_code("CFFEX", "IF2109");
    std::cout << "   ✓ make_code(\"CFFEX\", \"IF2109\") 成功，code = " << code1.get_symbol() << std::endl;
    
    // 测试 const char* 参数
    lt::code_t code2 = lt::make_code("CFFEX", "IF2109");
    std::cout << "   ✓ make_code(\"CFFEX\", \"IF2109\") 成功，code = " << code2.get_symbol() << std::endl;
    
    // 测试混合参数
    lt::code_t code3 = lt::make_code("CFFEX", "IF2109");
    std::cout << "   ✓ make_code(\"CFFEX\", \"IF2109\") 成功，code = " << code3.get_symbol() << std::endl;
    
    // 测试 value_to_string 函数
    std::cout << "\n2. 测试 value_to_string 函数:" << std::endl;
    
    // 测试整数
    std::string int_str = lt::value_to_string(42);
    std::cout << "   ✓ value_to_string(42) = " << int_str << std::endl;
    
    // 测试浮点数
    std::string float_str = lt::value_to_string(3.14159);
    std::cout << "   ✓ value_to_string(3.14159) = " << float_str << std::endl;
    
    // 测试 double
    std::string double_str = lt::value_to_string(2.718281828459045);
    std::cout << "   ✓ value_to_string(2.718281828459045) = " << double_str << std::endl;
    
    // 测试 pairarr_to_string 函数
    std::cout << "\n3. 测试 pairarr_to_string 函数:" << std::endl;
    
    std::array<std::pair<int, int>, 3> pairs = {
        std::make_pair(1, 10),
        std::make_pair(2, 20),
        std::make_pair(3, 30)
    };
    
    std::string pair_str = lt::pairarr_to_string(pairs);
    std::cout << "   ✓ pairarr_to_string() = " << pair_str << std::endl;
    
    // 测试带过滤回调的 pairarr_to_string
    auto filter = [](int key, int value) { return value > 15; };
    std::string filtered_str = lt::pairarr_to_string(pairs, filter);
    std::cout << "   ✓ pairarr_to_string() 带过滤 = " << filtered_str << std::endl;
    
    // 测试 string_to_pairarr 函数
    std::cout << "\n4. 测试 string_to_pairarr 函数:" << std::endl;
    
    std::string pairarr_str = "1:10,2:20,3:30";
    auto parsed_pairs = lt::string_to_pairarr<int, int, 3>(pairarr_str);
    std::cout << "   ✓ string_to_pairarr() 成功" << std::endl;
    for (auto& p : parsed_pairs) {
        std::cout << "     " << p.first << ":" << p.second << std::endl;
    }
    
    // 测试 valmap_to_string 函数
    std::cout << "\n5. 测试 valmap_to_string 函数:" << std::endl;
    
    std::map<int, double> valmap = {
        {1, 10.5},
        {2, 20.75},
        {3, 30.25}
    };
    
    std::string valmap_str = lt::valmap_to_string(valmap);
    std::cout << "   ✓ valmap_to_string() = " << valmap_str << std::endl;
    
    // 测试带过滤回调的 valmap_to_string
    auto valmap_filter = [](int key, double value) { return value > 15.0; };
    std::string filtered_valmap_str = lt::valmap_to_string(valmap, valmap_filter);
    std::cout << "   ✓ valmap_to_string() 带过滤 = " << filtered_valmap_str << std::endl;
    
    // 测试 string_to_valmap 函数
    std::cout << "\n6. 测试 string_to_valmap 函数:" << std::endl;
    
    std::string valmap_str_input = "1:10.5,2:20.75,3:30.25";
    auto parsed_valmap = lt::string_to_valmap<int, double>(valmap_str_input);
    std::cout << "   ✓ string_to_valmap() 成功" << std::endl;
    for (auto& p : parsed_valmap) {
        std::cout << "     " << p.first << ":" << p.second << std::endl;
    }
    
    // 测试 get_deal_direction 函数
    std::cout << "\n7. 测试 get_deal_direction 函数:" << std::endl;
    
    lt::tick_info prev_tick;
    prev_tick.price = 4500.0;
    prev_tick.bid_price[0] = 4490.0;
    prev_tick.ask_price[0] = 4510.0;
    
    lt::tick_info up_tick;
    up_tick.price = 4520.0;
    up_tick.bid_price[0] = 4500.0;
    up_tick.ask_price[0] = 4520.0;
    
    lt::tick_info down_tick;
    down_tick.price = 4480.0;
    down_tick.bid_price[0] = 4480.0;
    down_tick.ask_price[0] = 4500.0;
    
    lt::tick_info flat_tick;
    flat_tick.price = 4500.0;
    flat_tick.bid_price[0] = 4490.0;
    flat_tick.ask_price[0] = 4510.0;
    
    lt::deal_direction dir_up = lt::get_deal_direction(prev_tick, up_tick);
    std::cout << "   ✓ get_deal_direction(up) = " << static_cast<int>(dir_up) << std::endl;
    
    lt::deal_direction dir_down = lt::get_deal_direction(prev_tick, down_tick);
    std::cout << "   ✓ get_deal_direction(down) = " << static_cast<int>(dir_down) << std::endl;
    
    lt::deal_direction dir_flat = lt::get_deal_direction(prev_tick, flat_tick);
    std::cout << "   ✓ get_deal_direction(flat) = " << static_cast<int>(dir_flat) << std::endl;
    
    std::cout << "=== basic_utils 测试完成 ===\n" << std::endl;
}

int main() {
    std::cout << "=== 开始 basic_utils 模块单元测试 ===\n" << std::endl;
    
    test_basic_utils();
    
    std::cout << "=== basic_utils 模块单元测试完成 ===" << std::endl;
    return 0;
}
