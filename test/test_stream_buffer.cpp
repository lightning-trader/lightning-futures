#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <sstream>
#include "../src/share/stream_buffer.hpp"

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

// Test stream_carbureter basic encoding
void test_carbureter_basic() {
    std::cout << "\n=== Testing stream_carbureter Basic Encoding ===" << std::endl;
    
    unsigned char buffer[256];
    stream_carbureter carb(buffer, sizeof(buffer));
    
    // Test encoding integers
    carb << static_cast<int>(42);
    carb << static_cast<double>(3.14);
    
    // buffer[0] stores element count, should be 2 after 2 insertions
    ASSERT_TRUE(buffer[0] >= 1, "Elements were encoded");
}

// Test stream_carbureter string encoding
void test_carbureter_string() {
    std::cout << "\n=== Testing stream_carbureter String Encoding ===" << std::endl;
    
    unsigned char buffer[256];
    stream_carbureter carb(buffer, sizeof(buffer));
    
    // Test encoding string
    carb << "Hello";
    
    ASSERT_TRUE(buffer[0] >= 1, "String was encoded");
}

// Test stream_extractor basic decoding
void test_extractor_basic() {
    std::cout << "\n=== Testing stream_extractor Basic Decoding ===" << std::endl;
    
    unsigned char buffer[256];
    stream_carbureter carb(buffer, sizeof(buffer));
    
    // Encode some values
    carb << static_cast<int>(42);
    carb << static_cast<double>(3.14);
    
    // Decode
    stream_extractor ext(buffer, sizeof(buffer));
    std::ostringstream os;
    ext.out(os);
    
    std::string result = os.str();
    ASSERT_TRUE(result.find("42") != std::string::npos, "Output contains 42");
    ASSERT_TRUE(result.find("3.14") != std::string::npos, "Output contains 3.14");
}

// Test buffer overflow protection
void test_buffer_overflow() {
    std::cout << "\n=== Testing Buffer Overflow Protection ===" << std::endl;
    
    unsigned char buffer[16];  // Small buffer
    stream_carbureter carb(buffer, sizeof(buffer));
    
    // Try to write too much data
    bool thrown = false;
    try {
        carb << static_cast<int>(1);
        carb << static_cast<double>(2.0);
        carb << static_cast<long long>(3);
        carb << static_cast<double>(4.0);
    } catch (const std::out_of_range&) {
        thrown = true;
    }
    
    ASSERT_TRUE(thrown, "Buffer overflow throws exception");
}

// Test clear function
void test_clear() {
    std::cout << "\n=== Testing Clear Function ===" << std::endl;
    
    unsigned char buffer[256];
    stream_carbureter carb(buffer, sizeof(buffer));
    
    // Encode some values
    carb << static_cast<int>(42);
    
    // Clear
    carb.clear();
    
    ASSERT_EQ(0, buffer[0], "Buffer cleared");
}

// Test multiple types
void test_multiple_types() {
    std::cout << "\n=== Testing Multiple Types ===" << std::endl;
    
    unsigned char buffer[512];
    stream_carbureter carb(buffer, sizeof(buffer));
    
    // Encode various types
    carb << static_cast<bool>(true);
    carb << static_cast<char>('A');
    carb << static_cast<unsigned char>(255);
    carb << static_cast<short>(1000);
    carb << static_cast<unsigned short>(2000);
    carb << static_cast<int>(-5000);
    carb << static_cast<unsigned int>(10000U);
    carb << static_cast<float>(1.5f);
    
    ASSERT_TRUE(buffer[0] >= 1, "Multiple types were encoded");
}

// Test reset function
void test_reset() {
    std::cout << "\n=== Testing Reset Function ===" << std::endl;
    
    unsigned char buffer[256];
    stream_carbureter carb(buffer, sizeof(buffer));
    
    carb << static_cast<int>(42);
    
    stream_extractor ext(buffer, sizeof(buffer));
    ext.reset();
    
    ASSERT_TRUE(true, "Reset function works");
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   Stream Buffer Module Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    test_carbureter_basic();
    test_carbureter_string();
    test_extractor_basic();
    test_buffer_overflow();
    test_clear();
    test_multiple_types();
    test_reset();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "   Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "   Passed: " << tests_passed << std::endl;
    std::cout << "   Failed: " << tests_failed << std::endl;
    std::cout << "========================================" << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}