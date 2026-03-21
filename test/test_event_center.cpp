#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include "../src/share/event_center.hpp"

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

// Event type enum
enum class TestEventType {
    EVENT_A = 0,
    EVENT_B = 1,
    EVENT_C = 2
};

// Test queue_event_source basic functionality
void test_queue_event_source_basic() {
    std::cout << "\n=== Testing queue_event_source Basic ===" << std::endl;
    
    lt::queue_event_source<TestEventType, 16> event_source;
    
    // Test initial state
    ASSERT_TRUE(event_source.is_empty(), "Initial state is empty");
    ASSERT_FALSE(event_source.is_full(), "Initial state is not full");
}

// Test add_handle and fire_event
void test_add_handle_fire_event() {
    std::cout << "\n=== Testing add_handle and fire_event ===" << std::endl;
    
    lt::queue_event_source<TestEventType, 16> event_source;
    
    bool event_triggered = false;
    
    // Add event handler
    event_source.add_handle(TestEventType::EVENT_A, 
        [&event_triggered](const std::vector<std::any>& params) {
            event_triggered = true;
        });
    
    // Fire event
    event_source.fire_event(TestEventType::EVENT_A);
    
    // Poll to process event
    event_source.polling();
    
    ASSERT_TRUE(event_triggered, "Event handler was called");
}

// Test multiple event handlers
void test_multiple_handlers() {
    std::cout << "\n=== Testing Multiple Event Handlers ===" << std::endl;
    
    lt::queue_event_source<TestEventType, 16> event_source;
    
    int counter = 0;
    
    // Add multiple handlers for same event type
    event_source.add_handle(TestEventType::EVENT_A, 
        [&counter](const std::vector<std::any>& params) {
            counter++;
        });
    
    event_source.add_handle(TestEventType::EVENT_A, 
        [&counter](const std::vector<std::any>& params) {
            counter++;
        });
    
    // Fire event
    event_source.fire_event(TestEventType::EVENT_A);
    event_source.polling();
    
    ASSERT_EQ(2, counter, "Both handlers were called");
}

// Test different event types
void test_different_event_types() {
    std::cout << "\n=== Testing Different Event Types ===" << std::endl;
    
    lt::queue_event_source<TestEventType, 16> event_source;
    
    bool event_a_triggered = false;
    bool event_b_triggered = false;
    
    // Add handlers for different event types
    event_source.add_handle(TestEventType::EVENT_A, 
        [&event_a_triggered](const std::vector<std::any>& params) {
            event_a_triggered = true;
        });
    
    event_source.add_handle(TestEventType::EVENT_B, 
        [&event_b_triggered](const std::vector<std::any>& params) {
            event_b_triggered = true;
        });
    
    // Fire EVENT_A only
    event_source.fire_event(TestEventType::EVENT_A);
    event_source.polling();
    
    ASSERT_TRUE(event_a_triggered, "EVENT_A handler was called");
    ASSERT_FALSE(event_b_triggered, "EVENT_B handler was not called");
}

// Test event with parameters
void test_event_with_params() {
    std::cout << "\n=== Testing Event With Parameters ===" << std::endl;
    
    lt::queue_event_source<TestEventType, 16> event_source;
    
    int received_value = 0;
    
    // Add handler that receives parameters
    event_source.add_handle(TestEventType::EVENT_A, 
        [&received_value](const std::vector<std::any>& params) {
            if (!params.empty()) {
                received_value = std::any_cast<int>(params[0]);
            }
        });
    
    // Fire event with parameter
    event_source.fire_event(TestEventType::EVENT_A, 42);
    event_source.polling();
    
    ASSERT_EQ(42, received_value, "Parameter value received correctly");
}

// Test clear_handle
void test_clear_handle() {
    std::cout << "\n=== Testing clear_handle ===" << std::endl;
    
    lt::queue_event_source<TestEventType, 16> event_source;
    
    bool event_triggered = false;
    
    // Add handler
    event_source.add_handle(TestEventType::EVENT_A, 
        [&event_triggered](const std::vector<std::any>& params) {
            event_triggered = true;
        });
    
    // Clear handlers
    event_source.clear_handle();
    
    // Fire event
    event_source.fire_event(TestEventType::EVENT_A);
    event_source.polling();
    
    ASSERT_FALSE(event_triggered, "Handler was cleared and not called");
}

// Test direct_event_source
void test_direct_event_source() {
    std::cout << "\n=== Testing direct_event_source ===" << std::endl;
    
    lt::direct_event_source<TestEventType> event_source;
    
    bool event_triggered = false;
    
    // Add handler
    event_source.add_handle(TestEventType::EVENT_A, 
        [&event_triggered](const std::vector<std::any>& params) {
            event_triggered = true;
        });
    
    // Fire event (direct, no polling needed)
    event_source.fire_event(TestEventType::EVENT_A);
    
    ASSERT_TRUE(event_triggered, "Direct event handler was called immediately");
}

// Test is_full
void test_is_full() {
    std::cout << "\n=== Testing is_full ===" << std::endl;
    
    lt::queue_event_source<TestEventType, 4> event_source;  // Small buffer
    
    // Fill the queue
    for (int i = 0; i < 4; i++) {
        event_source.fire_event(TestEventType::EVENT_A);
    }
    
    ASSERT_TRUE(event_source.is_full(), "Queue is full");
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   Event Center Module Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    test_queue_event_source_basic();
    test_add_handle_fire_event();
    test_multiple_handlers();
    test_different_event_types();
    test_event_with_params();
    test_clear_handle();
    test_direct_event_source();
    test_is_full();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "   Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "   Passed: " << tests_passed << std::endl;
    std::cout << "   Failed: " << tests_failed << std::endl;
    std::cout << "========================================" << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}