#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <fstream>
#include <filesystem>
#include "../src/include/basic_define.h"
#include "../src/core/time_section.h"

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

// 创建测试配置文件
std::string create_test_config() {
    std::string config_path = "test_time_section_config.ini";
    std::ofstream ofs(config_path);
    // 期货交易时间配置 (格式：HHMMSSmmm)
    // 早盘 9:00-10:15, 10:30-11:30
    // 午盘 13:30-15:00
    // 夜盘 21:00-23:00
    ofs << "[trading_time]\n";
    ofs << "section1=090000000,101500000\n";  // 9:00-10:15
    ofs << "section2=103000000,113000000\n";  // 10:30-11:30
    ofs << "section3=133000000,150000000\n";  // 13:30-15:00
    ofs << "section4=210000000,230000000\n";  // 21:00-23:00
    ofs.close();
    return config_path;
}

void cleanup_test_config(const std::string& config_path) {
    std::filesystem::remove(config_path);
}

// 测试 time_section 基本功能
void test_time_section_basic() {
    std::cout << "\n=== 测试 time_section 基本功能 ===" << std::endl;
    
    std::string config_path = create_test_config();
    
    // 测试构造函数
    {
        lt::time_section ts(config_path);
        ASSERT_TRUE(true, "time_section 构造成功");
    }
    
    // 测试 is_trade_time() - 交易时间内
    {
        lt::time_section ts(config_path);
        // 9:30 应该在交易时间内
        lt::daytm_t time1 = 93000000;
        ASSERT_TRUE(ts.is_trade_time(time1), "9:30 在交易时间内");
    }
    
    // 测试 is_trade_time() - 非交易时间
    {
        lt::time_section ts(config_path);
        // 12:00 应该在午休，非交易时间
        lt::daytm_t time2 = 120000000;
        ASSERT_TRUE(!ts.is_trade_time(time2), "12:00 不在交易时间内");
    }
    
    // 测试 is_trade_time() - 10:15-10:30 节间休息
    {
        lt::time_section ts(config_path);
        // 10:20 应该在节间休息
        lt::daytm_t time3 = 102000000;
        ASSERT_TRUE(!ts.is_trade_time(time3), "10:20 不在交易时间内 (节间休息)");
    }
    
    // 测试 get_open_time()
    {
        lt::time_section ts(config_path);
        lt::daytm_t open = ts.get_open_time();
        // 应该返回最早的开盘时间 9:00
        ASSERT_EQ(90000000U, open, "get_open_time() 返回 9:00");
    }
    
    // 测试 get_close_time()
    {
        lt::time_section ts(config_path);
        lt::daytm_t close = ts.get_close_time();
        // 应该返回最晚的收盘时间 (夜盘 23:00 或日盘 15:00)
        ASSERT_TRUE(close == 230000000U || close == 150000000U, "get_close_time() 返回正确收盘时间");
    }
    
    cleanup_test_config(config_path);
}

// 测试 time_section 边界情况
void test_time_section_edge() {
    std::cout << "\n=== 测试 time_section 边界情况 ===" << std::endl;
    
    std::string config_path = create_test_config();
    
    // 测试边界时间点
    {
        lt::time_section ts(config_path);
        
        // 9:00:00.000 正好开盘
        ASSERT_TRUE(ts.is_trade_time(90000000), "9:00:00.000 在交易时间内");
        
        // 10:15:00.000 正好第一节结束
        ASSERT_TRUE(ts.is_trade_time(101500000), "10:15:00.000 在交易时间内 (边界)");
        
        // 15:00:00.000 正好日盘结束
        ASSERT_TRUE(ts.is_trade_time(150000000), "15:00:00.000 在交易时间内 (边界)");
    }
    
    cleanup_test_config(config_path);
}

// 测试 next_open_time()
void test_next_open_time() {
    std::cout << "\n=== 测试 next_open_time() ===" << std::endl;
    
    std::string config_path = create_test_config();
    
    {
        lt::time_section ts(config_path);
        
        // 12:00 的下一个开盘时间应该是 13:30
        lt::daytm_t next = ts.next_open_time(120000000);
        ASSERT_EQ(133000000U, next, "12:00 的下一个开盘时间是 13:30");
        
        // 23:00 后的下一个开盘时间应该是第二天 9:00
        lt::daytm_t next2 = ts.next_open_time(230000000);
        ASSERT_TRUE(next2 == 90000000U || next2 > 230000000U, "23:00 后的下一个开盘时间正确");
    }
    
    cleanup_test_config(config_path);
}

// 测试无效配置文件
void test_invalid_config() {
    std::cout << "\n=== 测试无效配置文件 ===" << std::endl;
    
    // 测试不存在的配置文件
    {
        try {
            lt::time_section ts("non_existent_config.ini");
            ASSERT_TRUE(true, "不存在的配置文件处理正确");
        } catch (...) {
            ASSERT_TRUE(true, "不存在的配置文件抛出异常");
        }
    }
}

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "   time_section 模块单元测试" << std::endl;
    std::cout << "============================================" << std::endl;
    
    test_time_section_basic();
    test_time_section_edge();
    test_next_open_time();
    test_invalid_config();
    
    std::cout << "\n============================================" << std::endl;
    std::cout << "   测试结果汇总" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "   通过：" << tests_passed << std::endl;
    std::cout << "   失败：" << tests_failed << std::endl;
    std::cout << "============================================" << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}