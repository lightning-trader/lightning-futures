#include <iostream>
#include "../src/include/params.hpp"

// 测试 params 类
void test_params() {
    std::cout << "=== 测试 params 类 ===" << std::endl;
    
    // 测试默认构造函数
    std::cout << "1. 测试默认构造函数:" << std::endl;
    lt::params params1;
    std::cout << "   ✓ params 默认构造函数成功" << std::endl;
    
    // 测试带参数的构造函数
    std::cout << "\n2. 测试带参数的构造函数:" << std::endl;
    
    // 测试从字符串构造
    lt::params params2("interval=100&initial_capital=1000000.0&contract_config=../bin/config/contract.csv");
    std::cout << "   ✓ params 从字符串构造成功" << std::endl;
    
    // 测试从 map 构造
    std::map<std::string, std::string> config_map = {
        {"interval", "100"},
        {"initial_capital", "1000000.0"},
        {"contract_config", "../bin/config/contract.csv"}
    };
    lt::params params3(config_map);
    std::cout << "   ✓ params 从 map 构造成功" << std::endl;
    
    // 测试 set_data 方法
    std::cout << "\n3. 测试 set_data 方法:" << std::endl;
    lt::params params4;
    params4.set_data(config_map);
    std::cout << "   ✓ set_data 成功" << std::endl;
    
    // 测试 data 方法
    std::cout << "\n4. 测试 data 方法:" << std::endl;
    auto data = params4.data();
    std::cout << "   ✓ data() 成功，size = " << data.size() << std::endl;
    for (const auto& pair : data) {
        std::cout << "     " << pair.first << " = " << pair.second << std::endl;
    }
    
    // 测试 has 方法
    std::cout << "\n5. 测试 has 方法:" << std::endl;
    bool has_interval = params4.has("interval");
    bool has_nonexistent = params4.has("nonexistent");
    std::cout << "   ✓ has(\"interval\") = " << has_interval << std::endl;
    std::cout << "   ✓ has(\"nonexistent\") = " << has_nonexistent << std::endl;
    
    // 测试 get 方法
    std::cout << "\n6. 测试 get 方法:" << std::endl;
    
    // 测试获取字符串
    std::string contract_config = params4.get<std::string>("contract_config");
    std::cout << "   ✓ get<std::string>(\"contract_config\") = " << contract_config << std::endl;
    
    // 测试获取整数
    int interval = params4.get<int32_t>("interval");
    std::cout << "   ✓ get<int32_t>(\"interval\") = " << interval << std::endl;
    
    // 测试获取浮点数
    double initial_capital = params4.get<double_t>("initial_capital");
    std::cout << "   ✓ get<double_t>(\"initial_capital\") = " << initial_capital << std::endl;
    
    // 测试获取布尔值
    lt::params bool_params("enable_debug=true&enable_logging=1&enable_testing=false");
    bool enable_debug = bool_params.get<bool>("enable_debug");
    bool enable_logging = bool_params.get<bool>("enable_logging");
    bool enable_testing = bool_params.get<bool>("enable_testing");
    std::cout << "   ✓ get<bool>(\"enable_debug\") = " << enable_debug << std::endl;
    std::cout << "   ✓ get<bool>(\"enable_logging\") = " << enable_logging << std::endl;
    std::cout << "   ✓ get<bool>(\"enable_testing\") = " << enable_testing << std::endl;
    
    // 测试获取不存在的键
    std::cout << "\n7. 测试获取不存在的键:" << std::endl;
    try {
        auto value = params4.get<std::string>("nonexistent");
        std::cout << "   ✗ 应该抛出异常" << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cout << "   ✓ 正确抛出 invalid_argument 异常: " << e.what() << std::endl;
    }
    
    std::cout << "=== params 测试完成 ===\n" << std::endl;
}

int main() {
    std::cout << "=== 开始 params 模块单元测试 ===\n" << std::endl;
    
    test_params();
    
    std::cout << "=== params 模块单元测试完成 ===" << std::endl;
    return 0;
}
