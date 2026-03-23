#include <iostream>
#include <vector>
#include <cassert>
#include "../src/share/string_helper.hpp"

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

// 测试 split 函数
void test_split() {
    std::cout << "\n=== 测试 split 函数 ===" << std::endl;
    
    // 测试基本分割
    {
        std::string str = "a,b,c,d";
        auto result = lt::string_helper::split(str, ',');
        ASSERT_EQ(4, result.size(), "split 基本分割数量正确");
        ASSERT_EQ(std::string("a"), result[0], "split 第一个元素正确");
        ASSERT_EQ(std::string("b"), result[1], "split 第二个元素正确");
        ASSERT_EQ(std::string("c"), result[2], "split 第三个元素正确");
        ASSERT_EQ(std::string("d"), result[3], "split 第四个元素正确");
    }
    
    // 测试空字符串
    {
        std::string str = "";
        auto result = lt::string_helper::split(str, ',');
        ASSERT_EQ(0, result.size(), "split 空字符串返回空数组");
    }
    
    // 测试单个元素
    {
        std::string str = "single";
        auto result = lt::string_helper::split(str, ',');
        ASSERT_EQ(1, result.size(), "split 单个元素返回正确数量");
        ASSERT_EQ(std::string("single"), result[0], "split 单个元素值正确");
    }
    
    // 测试连续分隔符
    {
        std::string str = "a,,b";
        auto result = lt::string_helper::split(str, ',');
        ASSERT_EQ(3, result.size(), "split 连续分隔符返回正确数量");
        ASSERT_EQ(std::string(""), result[1], "split 连续分隔符中间为空");
    }
}

// 测试 to_string 函数
void test_to_string() {
    std::cout << "\n=== 测试 to_string 函数 ===" << std::endl;
    
    // 测试整数
    {
        int val = 42;
        auto result = lt::string_helper::to_string(val);
        ASSERT_EQ(std::string("42"), result, "to_string 整数转换正确");
    }
    
    // 测试负数
    {
        int val = -42;
        auto result = lt::string_helper::to_string(val);
        ASSERT_EQ(std::string("-42"), result, "to_string 负数转换正确");
    }
    
    // 测试浮点数（带精度）
    {
        double val = 3.14159;
        auto result = lt::string_helper::to_string(val, 2);
        ASSERT_EQ(std::string("3.14"), result, "to_string 浮点数带精度转换正确");
    }
    
    // 测试字符串
    {
        std::string val = "hello";
        auto result = lt::string_helper::to_string(val);
        ASSERT_EQ(std::string("hello"), result, "to_string 字符串转换正确");
    }
    
    // 测试 C 字符串
    {
        const char* val = "world";
        auto result = lt::string_helper::to_string(val);
        ASSERT_EQ(std::string("world"), result, "to_string C 字符串转换正确");
    }
    
    // 测试向量
    {
        std::vector<int> val = {1, 2, 3};
        auto result = lt::string_helper::to_string(val);
        ASSERT_EQ(std::string("[1,2,3]"), result, "to_string 向量转换正确");
    }
}

// 测试 contains 函数
void test_contains() {
    std::cout << "\n=== 测试 contains 函数 ===" << std::endl;
    
    // 测试存在的元素
    {
        std::vector<int> vec = {1, 2, 3, 4, 5};
        ASSERT_TRUE(lt::string_helper::contains(vec, 3), "contains 找到存在的元素");
    }
    
    // 测试不存在的元素
    {
        std::vector<int> vec = {1, 2, 3, 4, 5};
        ASSERT_TRUE(!lt::string_helper::contains(vec, 6), "contains 找不到不存在的元素");
    }
    
    // 测试空向量
    {
        std::vector<int> vec = {};
        ASSERT_TRUE(!lt::string_helper::contains(vec, 1), "contains 空向量返回 false");
    }
    
    // 测试字符串向量
    {
        std::vector<std::string> vec = {"a", "b", "c"};
        ASSERT_TRUE(lt::string_helper::contains(vec, std::string("b")), "contains 字符串向量找到元素");
    }
}

// 测试 join 函数
void test_join() {
    std::cout << "\n=== 测试 join 函数 ===" << std::endl;
    
    // 测试整数向量
    {
        std::vector<int> vec = {1, 2, 3, 4};
        auto result = lt::string_helper::join(',', vec);
        ASSERT_EQ(std::string("1,2,3,4"), result, "join 整数向量正确");
    }
    
    // 测试空向量
    {
        std::vector<int> vec = {};
        auto result = lt::string_helper::join(',', vec);
        ASSERT_EQ(std::string(""), result, "join 空向量返回空字符串");
    }
    
    // 测试单个元素
    {
        std::vector<int> vec = {42};
        auto result = lt::string_helper::join(',', vec);
        ASSERT_EQ(std::string("42"), result, "join 单个元素正确");
    }
    
    // 测试字符串向量
    {
        std::vector<std::string> vec = {"a", "b", "c"};
        auto result = lt::string_helper::join('|', vec);
        ASSERT_EQ(std::string("a|b|c"), result, "join 字符串向量正确");
    }
}

// 测试 format 函数
void test_format() {
    std::cout << "\n=== 测试 format 函数 ===" << std::endl;
    
    // 测试基本格式化
    {
        std::string fmt = "Hello, {}!";
        auto result = lt::string_helper::format(fmt, "World");
        ASSERT_EQ(std::string("Hello, World!"), result, "format 基本格式化正确");
    }
    
    // 测试多个参数
    {
        std::string fmt = "{} + {} = {}";
        auto result = lt::string_helper::format(fmt, 1, 2, 3);
        ASSERT_EQ(std::string("1 + 2 = 3"), result, "format 多个参数正确");
    }
    
    // 测试无参数
    {
        std::string fmt = "No placeholders";
        auto result = lt::string_helper::format(fmt);
        ASSERT_EQ(std::string("No placeholders"), result, "format 无参数正确");
    }
    
    // 测试混合类型
    {
        std::string fmt = "Int: {}, Double: {}, String: {}";
        auto result = lt::string_helper::format(fmt, 42, 3.14, "test");
        ASSERT_EQ(std::string("Int: 42, Double: 3.14, String: test"), result, "format 混合类型正确");
    }
}

// 测试 extract_to_string 函数
void test_extract_to_string() {
    std::cout << "\n=== 测试 extract_to_string 函数 ===" << std::endl;
    
    // 测试单个值提取
    {
        int val = 42;
        auto result = lt::string_helper::extract_to_string(val);
        ASSERT_EQ(std::string("42"), result, "extract_to_string 单个值正确");
    }
    
    // 测试多个值提取
    {
        std::vector<std::string> data;
        lt::string_helper::extract_to_string(data, 1, 2.5, "test");
        ASSERT_EQ(3, data.size(), "extract_to_string 多个值数量正确");
        ASSERT_EQ(std::string("1"), data[0], "extract_to_string 第一个值正确");
        ASSERT_EQ(std::string("2.5"), data[1], "extract_to_string 第二个值正确");
        ASSERT_EQ(std::string("test"), data[2], "extract_to_string 第三个值正确");
    }
}

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "   string_helper 模块单元测试" << std::endl;
    std::cout << "============================================" << std::endl;
    
    test_split();
    test_to_string();
    test_contains();
    test_join();
    test_format();
    test_extract_to_string();
    
    std::cout << "\n============================================" << std::endl;
    std::cout << "   测试结果汇总" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "   通过：" << tests_passed << std::endl;
    std::cout << "   失败：" << tests_failed << std::endl;
    std::cout << "============================================" << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}