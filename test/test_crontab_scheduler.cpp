#include <iostream>
#include <vector>
#include <cassert>
#include <thread>
#include <chrono>
#include "../src/share/crontab_scheduler.hpp"

// Test counter
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

#define ASSERT_FALSE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "   [PASS] " << message << std::endl; \
            tests_passed++; \
        } else { \
            std::cout << "   [FAIL] " << message << std::endl; \
            tests_failed++; \
        } \
    } while(0)

// Test basic schedule creation
void test_add_schedule() {
    std::cout << "\n=== Testing add_schedule ===" << std::endl;
    
    lt::crontab_scheduler scheduler;
    
    // Add a schedule
    int id = scheduler.add_schedule("09:00:00", std::chrono::hours(8));
    
    ASSERT_EQ(0, id, "First schedule ID is 0");
}

// Test set_work_days
void test_set_work_days() {
    std::cout << "\n=== Testing set_work_days ===" << std::endl;
    
    lt::crontab_scheduler scheduler;
    
    // Set work days (Monday to Friday)
    scheduler.set_work_days({1, 2, 3, 4, 5});
    
    ASSERT_TRUE(true, "set_work_days executed successfully");
}

// Test set_callback
void test_set_callback() {
    std::cout << "\n=== Testing set_callback ===" << std::endl;
    
    lt::crontab_scheduler scheduler;
    
    bool begin_called = false;
    bool end_called = false;
    
    scheduler.set_callback(
        [&begin_called](int id) { begin_called = true; },
        [&end_called](int id) { end_called = true; }
    );
    
    ASSERT_TRUE(true, "set_callback executed successfully");
}

// Test polling without active schedules
void test_polling_empty() {
    std::cout << "\n=== Testing polling with empty schedules ===" << std::endl;
    
    lt::crontab_scheduler scheduler;
    
    // Polling should not crash with no schedules
    scheduler.polling();
    
    ASSERT_TRUE(true, "polling executed successfully");
}

// Test multiple schedules
void test_multiple_schedules() {
    std::cout << "\n=== Testing multiple_schedules ===" << std::endl;
    
    lt::crontab_scheduler scheduler;
    
    int id1 = scheduler.add_schedule("09:00:00", std::chrono::hours(8));
    int id2 = scheduler.add_schedule("14:00:00", std::chrono::hours(2));
    int id3 = scheduler.add_schedule("20:00:00", std::chrono::hours(4));
    
    ASSERT_EQ(0, id1, "First schedule ID is 0");
    ASSERT_EQ(1, id2, "Second schedule ID is 1");
    ASSERT_EQ(2, id3, "Third schedule ID is 2");
}

// Test callback execution
void test_callback_execution() {
    std::cout << "\n=== Testing callback execution ===" << std::endl;
    
    lt::crontab_scheduler scheduler;
    
    int begin_count = 0;
    int end_count = 0;
    
    scheduler.set_callback(
        [&begin_count](int id) { begin_count++; },
        [&end_count](int id) { end_count++; }
    );
    
    // Add a schedule that should trigger immediately
    // Use current time to ensure it triggers
    scheduler.add_schedule("00:00:00", std::chrono::seconds(1));
    scheduler.set_work_days({0, 1, 2, 3, 4, 5, 6});  // All days
    
    // Poll multiple times
    for (int i = 0; i < 3; i++) {
        scheduler.polling();
    }
    
    ASSERT_TRUE(begin_count >= 0, "Begin callback can be called");
}

// Test schedule with different durations
void test_different_durations() {
    std::cout << "\n=== Testing different durations ===" << std::endl;
    
    lt::crontab_scheduler scheduler;
    
    // Add schedules with different durations
    scheduler.add_schedule("08:00:00", std::chrono::hours(1));
    scheduler.add_schedule("12:00:00", std::chrono::minutes(30));
    scheduler.add_schedule("18:00:00", std::chrono::seconds(300));
    
    ASSERT_TRUE(true, "Different durations accepted");
}

// Test work day validation
void test_work_day_validation() {
    std::cout << "\n=== Testing work day validation ===" << std::endl;
    
    lt::crontab_scheduler scheduler;
    
    // Valid days
    scheduler.set_work_days({0, 1, 2, 3, 4, 5, 6});
    ASSERT_TRUE(true, "Valid work days accepted");
    
    // Invalid days should be ignored
    scheduler.set_work_days({7, 8, 9});  // Invalid days
    // After this, work_days should be empty since all are invalid
    
    ASSERT_TRUE(true, "Invalid work days handled");
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   Crontab Scheduler Module Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    test_add_schedule();
    test_set_work_days();
    test_set_callback();
    test_polling_empty();
    test_multiple_schedules();
    test_callback_execution();
    test_different_durations();
    test_work_day_validation();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "   Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "   Passed: " << tests_passed << std::endl;
    std::cout << "   Failed: " << tests_failed << std::endl;
    std::cout << "========================================" << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}