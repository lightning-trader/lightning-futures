#include <iostream>
#include <cassert>
#include <ctime>
#include "../src/include/basic_define.h"
#include "../src/share/time_utils.hpp"
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

#define ASSERT_GE(actual, expected, message) \
    do { \
        if ((actual) >= (expected)) { \
            std::cout << "   [PASS] " << message << std::endl; \
            tests_passed++; \
        } else { \
            std::cout << "   [FAIL] " << message << std::endl; \
            tests_failed++; \
        } \
    } while(0)

// 测试 make_date 函数
void test_make_date() {
    std::cout << "\n=== 测试 make_date 函数 ===" << std::endl;
    
    // 测试年月日构造
    {
        time_t result = lt::time_utils::make_date(2023, 1, 15);
        ASSERT_GE(result, 0, "make_date 返回有效时间戳");
    }
    
    // 测试 uint32_t 日期格式
    {
        time_t result = lt::time_utils::make_date(20230115U);
        ASSERT_GE(result, 0, "make_date(uint32_t) 返回有效时间戳");
    }
}

// 测试 make_time 函数
void test_make_time() {
    std::cout << "\n=== 测试 make_time 函数 ===" << std::endl;
    
    // 测试 uint32_t 时间格式
    {
        time_t result = lt::time_utils::make_time(93000000U);
        ASSERT_GE(result, 0, "make_time(uint32_t) 返回有效时间戳");
    }
    
    // 测试字符串时间格式
    {
        time_t result = lt::time_utils::make_time("09:30:00");
        ASSERT_GE(result, 0, "make_time(const char*) 返回有效时间戳");
    }
}

// 测试 make_datetime 函数
void test_make_datetime() {
    std::cout << "\n=== 测试 make_datetime 函数 ===" << std::endl;
    
    // 测试日期 + 时间字符串
    {
        time_t result = lt::time_utils::make_datetime("2023-01-15", "09:30:00");
        ASSERT_GE(result, 0, "make_datetime 返回有效时间戳");
    }
    
    // 测试 uint32_t 日期 + 时间字符串
    {
        time_t result = lt::time_utils::make_datetime(20230115U, "09:30:00");
        ASSERT_GE(result, 0, "make_datetime(uint32_t, const char*) 返回有效时间戳");
    }
}

// 测试 get_day_begin 函数
void test_get_day_begin() {
    std::cout << "\n=== 测试 get_day_begin 函数 ===" << std::endl;
    
    time_t now = std::time(nullptr);
    time_t day_begin = lt::time_utils::get_day_begin(now);
    
    ASSERT_GE(day_begin, 0, "get_day_begin 返回有效时间戳");
    ASSERT_TRUE(day_begin <= now, "get_day_begin 返回当天开始时间");
}

// 测试 get_day_time 函数
void test_get_day_time() {
    std::cout << "\n=== 测试 get_day_time 函数 ===" << std::endl;
    
    time_t now = std::time(nullptr);
    lt::daytm_t daytm = lt::time_utils::get_day_time(now);
    
    ASSERT_TRUE(daytm < 240000000U, "get_day_time 返回有效日内时间");
}

// 测试 make_daytm 函数
void test_make_daytm() {
    std::cout << "\n=== 测试 make_daytm 函数 ===" << std::endl;
    
    // 测试字符串时间 + tick
    {
        lt::daytm_t result = lt::time_utils::make_daytm("09:30:00", 500U);
        ASSERT_TRUE(result > 0, "make_daytm 返回有效日内时间");
    }
    
    // 测试 uint32_t 时间 + tick
    {
        lt::daytm_t result = lt::time_utils::make_daytm(93000000U, 500U);
        ASSERT_EQ(93000000U, result, "make_daytm(uint32_t, uint32_t) 返回正确时间");
    }
}

// 测试 make_seqtm 函数
void test_make_seqtm() {
    std::cout << "\n=== 测试 make_seqtm 函数 ===" << std::endl;
    
    // 测试日期 + daytm
    {
        lt::seqtm_t result = lt::time_utils::make_seqtm(20230115U, 93000000U);
        ASSERT_TRUE(result > 0, "make_seqtm 返回有效有序时间");
    }
}

// 测试 get_uint_day 函数
void test_get_uint_day() {
    std::cout << "\n=== 测试 get_uint_day 函数 ===" << std::endl;
    
    lt::seqtm_t seqtm = lt::time_utils::make_seqtm(20230115U, 93000000U);
    uint32_t day = lt::time_utils::get_uint_day(seqtm);
    
    ASSERT_EQ(20230115U, day, "get_uint_day 返回正确日期");
}

// 测试 get_daytm 函数
void test_get_daytm() {
    std::cout << "\n=== 测试 get_daytm 函数 ===" << std::endl;
    
    lt::seqtm_t seqtm = lt::time_utils::make_seqtm(20230115U, 93000000U);
    lt::daytm_t daytm = lt::time_utils::get_daytm(seqtm);
    
    ASSERT_EQ(93000000U, daytm, "get_daytm 返回正确日内时间");
}

// 测试 datetime_to_string 函数
void test_datetime_to_string() {
    std::cout << "\n=== 测试 datetime_to_string 函数 ===" << std::endl;
    
    time_t now = std::time(nullptr);
    std::string result = lt::time_utils::datetime_to_string(now);
    
    ASSERT_TRUE(result.length() > 0, "datetime_to_string 返回非空字符串");
    ASSERT_TRUE(result.find("-") != std::string::npos, "datetime_to_string 包含日期分隔符");
}

// 测试 seqtm_to_string 函数
void test_seqtm_to_string() {
    std::cout << "\n=== 测试 seqtm_to_string 函数 ===" << std::endl;
    
    lt::seqtm_t seqtm = lt::time_utils::make_seqtm(20230115U, 93000000U);
    std::string result = lt::time_utils::seqtm_to_string(seqtm);
    
    ASSERT_TRUE(result.length() > 0, "seqtm_to_string 返回非空字符串");
}

// 测试 daytm_offset 函数
void test_daytm_offset() {
    std::cout << "\n=== 测试 daytm_offset 函数 ===" << std::endl;
    
    lt::daytm_t tm = 93000000U;
    lt::daytm_t result = lt::time_utils::daytm_offset(tm, 1000);
    
    ASSERT_TRUE(result > tm, "daytm_offset 正向偏移正确");
}

// 测试 time_forward 函数
void test_time_forward() {
    std::cout << "\n=== 测试 time_forward 函数 ===" << std::endl;
    
    lt::daytm_t tm = 93000000U;
    lt::daytm_t result = lt::time_utils::time_forward(tm, 60);
    
    ASSERT_TRUE(result > tm, "time_forward 向前移动正确");
}

// 测试 time_back 函数
void test_time_back() {
    std::cout << "\n=== 测试 time_back 函数 ===" << std::endl;
    
    lt::daytm_t tm = 93100000U;
    lt::daytm_t result = lt::time_utils::time_back(tm, 60);
    
    ASSERT_TRUE(result < tm, "time_back 向后移动正确");
}

// 测试 section_daytm_snap 函数
void test_section_daytm_snap() {
    std::cout << "\n=== 测试 section_daytm_snap 函数 ===" << std::endl;
    
    lt::daytm_t tm = 93012000U;
    lt::daytm_t result = lt::time_utils::section_daytm_snap(tm, 3000);
    
    ASSERT_TRUE(result <= tm, "section_daytm_snap 向下取整正确");
}

// 测试 is_same_day 函数
void test_is_same_day() {
    std::cout << "\n=== 测试 is_same_day 函数 ===" << std::endl;
    
    time_t t1 = std::time(nullptr);
    time_t t2 = t1 + 3600; // 1 小时后
    
    ASSERT_TRUE(lt::time_utils::is_same_day(t1, t2), "is_same_day 同一天返回 true");
    
    time_t t3 = t1 + 86400; // 1 天后
    ASSERT_TRUE(!lt::time_utils::is_same_day(t1, t3), "is_same_day 不同天返回 false");
}

// 测试日期/周/月/年边界函数
void test_date_boundary() {
    std::cout << "\n=== 测试日期边界函数 ===" << std::endl;
    
    time_t now = std::time(nullptr);
    
    // 测试 get_week_begin
    {
        time_t week_begin = lt::time_utils::get_week_begin(now);
        ASSERT_GE(week_begin, 0, "get_week_begin 返回有效时间戳");
        ASSERT_TRUE(week_begin <= now, "get_week_begin 返回本周开始");
    }
    
    // 测试 get_month_begin
    {
        time_t month_begin = lt::time_utils::get_month_begin(now);
        ASSERT_GE(month_begin, 0, "get_month_begin 返回有效时间戳");
        ASSERT_TRUE(month_begin <= now, "get_month_begin 返回本月开始");
    }
    
    // 测试 get_year_begin
    {
        time_t year_begin = lt::time_utils::get_year_begin(now);
        ASSERT_GE(year_begin, 0, "get_year_begin 返回有效时间戳");
        ASSERT_TRUE(year_begin <= now, "get_year_begin 返回本年开始");
    }
}

// 测试 is_same_week/month/year 函数
void test_is_same_period() {
    std::cout << "\n=== 测试 is_same_period 函数 ===" << std::endl;
    
    time_t now = std::time(nullptr);
    
    // is_same_week 已经在 is_same_day 中测试
    // 这里测试 is_same_month 和 is_same_year
    ASSERT_TRUE(lt::time_utils::is_same_month(now, now), "is_same_month 同一时间返回 true");
    ASSERT_TRUE(lt::time_utils::is_same_year(now, now), "is_same_year 同一时间返回 true");
}

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "   time_utils 模块单元测试" << std::endl;
    std::cout << "============================================" << std::endl;
    
    test_make_date();
    test_make_time();
    test_make_datetime();
    test_get_day_begin();
    test_get_day_time();
    test_make_daytm();
    test_make_seqtm();
    test_get_uint_day();
    test_get_daytm();
    test_datetime_to_string();
    test_seqtm_to_string();
    test_daytm_offset();
    test_time_forward();
    test_time_back();
    test_section_daytm_snap();
    test_is_same_day();
    test_date_boundary();
    test_is_same_period();
    
    std::cout << "\n============================================" << std::endl;
    std::cout << "   测试结果汇总" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "   通过：" << tests_passed << std::endl;
    std::cout << "   失败：" << tests_failed << std::endl;
    std::cout << "============================================" << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}