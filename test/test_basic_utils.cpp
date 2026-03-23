#include <iostream>
#include "../src/include/basic_utils.hpp"

// Test basic_utils module
void test_basic_utils() {
    std::cout << "=== Testing basic_utils Module ===" << std::endl;
    
    // Test make_code function
    std::cout << "1. Testing make_code function:" << std::endl;
    
    // Test string parameters
    lt::code_t code1 = lt::make_code("CFFEX", "IF2109");
    std::cout << "   make_code(\"CFFEX\", \"IF2109\") = " << code1.get_symbol() << std::endl;
    
    // Test const char* parameters
    lt::code_t code2 = lt::make_code("CFFEX", "IF2109");
    std::cout << "   make_code(\"CFFEX\", \"IF2109\") = " << code2.get_symbol() << std::endl;
    
    // Test value_to_string function
    std::cout << "\n2. Testing value_to_string function:" << std::endl;
    
    // Test integer
    std::string int_str = lt::value_to_string(42);
    std::cout << "   value_to_string(42) = " << int_str << std::endl;
    
    // Test floating point
    std::string float_str = lt::value_to_string(3.14159);
    std::cout << "   value_to_string(3.14159) = " << float_str << std::endl;
    
    // Test double
    std::string double_str = lt::value_to_string(2.718281828459045);
    std::cout << "   value_to_string(2.718281828459045) = " << double_str << std::endl;
    
    // Test pairarr_to_string function
    std::cout << "\n3. Testing pairarr_to_string function:" << std::endl;
    
    std::array<std::pair<int, int>, 3> pairs = {
        std::make_pair(1, 10),
        std::make_pair(2, 20),
        std::make_pair(3, 30)
    };
    
    std::string pair_str = lt::pairarr_to_string(pairs);
    std::cout << "   pairarr_to_string() = " << pair_str << std::endl;
    
    // Test with filter callback
    auto filter = [](int key, int value) { return value > 15; };
    std::string filtered_str = lt::pairarr_to_string<int, int, 3>(pairs, filter);
    std::cout << "   pairarr_to_string() with filter = " << filtered_str << std::endl;
    
    // Test string_to_pairarr function
    std::cout << "\n4. Testing string_to_pairarr function:" << std::endl;
    
    std::string pairarr_str = "1:10,2:20,3:30";
    auto parsed_pairs = lt::string_to_pairarr<int, int, 3>(pairarr_str);
    std::cout << "   string_to_pairarr() success" << std::endl;
    for (auto& p : parsed_pairs) {
        std::cout << "     " << p.first << ":" << p.second << std::endl;
    }
    
    // Test valmap_to_string function
    std::cout << "\n5. Testing valmap_to_string function:" << std::endl;
    
    std::map<int, double> valmap = {
        {1, 10.5},
        {2, 20.75},
        {3, 30.25}
    };
    
    std::string valmap_str = lt::valmap_to_string(valmap);
    std::cout << "   valmap_to_string() = " << valmap_str << std::endl;
    
    // Test with filter callback
    auto valmap_filter = [](int key, double value) { return value > 15.0; };
    std::string filtered_valmap_str = lt::valmap_to_string<int, double>(valmap, valmap_filter);
    std::cout << "   valmap_to_string() with filter = " << filtered_valmap_str << std::endl;
    
    // Test string_to_valmap function
    std::cout << "\n6. Testing string_to_valmap function:" << std::endl;
    
    std::string valmap_str_input = "1:10.5,2:20.75,3:30.25";
    auto parsed_valmap = lt::string_to_valmap<int, double>(valmap_str_input);
    std::cout << "   string_to_valmap() success" << std::endl;
    for (auto& p : parsed_valmap) {
        std::cout << "     " << p.first << ":" << p.second << std::endl;
    }
    
    // Test get_deal_direction function
    std::cout << "\n7. Testing get_deal_direction function:" << std::endl;
    
    lt::tick_info prev_tick;
    prev_tick.price = 4500.0;
    
    lt::tick_info up_tick;
    up_tick.price = 4520.0;
    
    lt::tick_info down_tick;
    down_tick.price = 4480.0;
    
    lt::tick_info flat_tick;
    flat_tick.price = 4500.0;
    
    lt::deal_direction dir_up = lt::get_deal_direction(prev_tick, up_tick);
    std::cout << "   get_deal_direction(up) = " << static_cast<int>(dir_up) << std::endl;
    
    lt::deal_direction dir_down = lt::get_deal_direction(prev_tick, down_tick);
    std::cout << "   get_deal_direction(down) = " << static_cast<int>(dir_down) << std::endl;
    
    lt::deal_direction dir_flat = lt::get_deal_direction(prev_tick, flat_tick);
    std::cout << "   get_deal_direction(flat) = " << static_cast<int>(dir_flat) << std::endl;
    
    std::cout << "=== basic_utils testing complete ===\n" << std::endl;
}

int main() {
    std::cout << "=== Starting basic_utils Module Unit Tests ===\n" << std::endl;
    
    test_basic_utils();
    
    std::cout << "=== basic_utils Module Unit Tests Complete ===" << std::endl;
    return 0;
}