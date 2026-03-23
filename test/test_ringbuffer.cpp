#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include "../src/share/ringbuffer.hpp"

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

// Test isEmpty and isFull
void test_empty_full() {
    std::cout << "\n=== Testing isEmpty/isFull Functions ===" << std::endl;
    
    Ringbuffer<int> rb;
    
    // Initial state should be empty
    ASSERT_TRUE(rb.isEmpty(), "Initial state is empty");
    ASSERT_FALSE(rb.isFull(), "Initial state is not full");
    
    // Fill with data
    for (int i = 0; i < 10; i++) {
        rb.insert(i);
    }
    
    ASSERT_FALSE(rb.isEmpty(), "Not empty after inserting data");
}

// Test insert and remove
void test_insert_remove() {
    std::cout << "\n=== Testing insert/remove Functions ===" << std::endl;
    
    Ringbuffer<int> rb;
    
    // Test insert and remove single element
    ASSERT_TRUE(rb.insert(42), "Insert element success");
    ASSERT_FALSE(rb.isEmpty(), "Not empty after insert");
    
    int value = 0;
    ASSERT_TRUE(rb.remove(value), "Remove element success");
    ASSERT_EQ(42, value, "Removed value is correct");
    ASSERT_TRUE(rb.isEmpty(), "Empty after remove");
    
    // Test multiple elements
    for (int i = 0; i < 5; i++) {
        rb.insert(i * 10);
    }
    
    for (int i = 0; i < 5; i++) {
        int val = 0;
        rb.remove(val);
        ASSERT_EQ(i * 10, val, "FIFO order correct");
    }
}

// Test peek function
void test_peek() {
    std::cout << "\n=== Testing peek Function ===" << std::endl;
    
    Ringbuffer<int> rb;
    
    // Peek on empty buffer should return nullptr
    ASSERT_TRUE(rb.peek() == nullptr, "Peek on empty buffer returns nullptr");
    
    // After insert, peek should return first element
    rb.insert(100);
    int* val = rb.peek();
    ASSERT_TRUE(val != nullptr, "Peek on non-empty buffer returns valid pointer");
    ASSERT_EQ(100, *val, "Peek returns correct value");
    
    // Peek should not remove element
    ASSERT_FALSE(rb.isEmpty(), "Not empty after peek");
}

// Test at function
void test_at() {
    std::cout << "\n=== Testing at Function ===" << std::endl;
    
    Ringbuffer<int> rb;
    
    // Insert multiple elements
    for (int i = 0; i < 10; i++) {
        rb.insert(i);
    }
    
    // Test accessing elements at different positions
    for (int i = 0; i < 10; i++) {
        int* val = rb.at(i);
        ASSERT_TRUE(val != nullptr, "at(" << i << ") returns valid pointer");
        ASSERT_EQ(i, *val, "at(" << i << ") returns correct value");
    }
}

// Test producerClear and consumerClear
void test_clear() {
    std::cout << "\n=== Testing clear Functions ===" << std::endl;
    
    Ringbuffer<int> rb;
    
    // Insert some data
    for (int i = 0; i < 10; i++) {
        rb.insert(i);
    }
    
    ASSERT_FALSE(rb.isEmpty(), "Not empty after inserting data");
    
    // Producer clear
    rb.producerClear();
    ASSERT_TRUE(rb.isEmpty(), "Empty after producerClear");
    
    // Insert data again
    for (int i = 0; i < 10; i++) {
        rb.insert(i);
    }
    
    // Consumer clear
    rb.consumerClear();
    ASSERT_TRUE(rb.isEmpty(), "Empty after consumerClear");
}

// Test readAvailable and writeAvailable
void test_available() {
    std::cout << "\n=== Testing available Functions ===" << std::endl;
    
    Ringbuffer<int> rb;
    
    // Initial state
    ASSERT_EQ(0, rb.readAvailable(), "Initial readable count is 0");
    
    // After inserting data
    for (int i = 0; i < 5; i++) {
        rb.insert(i);
    }
    ASSERT_EQ(5, rb.readAvailable(), "Readable count is 5 after inserting 5 elements");
}

// Test writeBuff and readBuff
void test_buff_io() {
    std::cout << "\n=== Testing writeBuff/readBuff Functions ===" << std::endl;
    
    Ringbuffer<int> rb;
    
    // Batch write
    int write_data[] = {1, 2, 3, 4, 5};
    size_t written = rb.writeBuff(write_data, 5);
    ASSERT_EQ(5, written, "Batch write count correct");
    
    // Batch read
    int read_data[5] = {0};
    size_t read = rb.readBuff(read_data, 5);
    ASSERT_EQ(5, read, "Batch read count correct");
    
    for (int i = 0; i < 5; i++) {
        ASSERT_EQ(write_data[i], read_data[i], "Batch read data correct [" << i << "]");
    }
}

// Test remove batch delete
void test_batch_remove() {
    std::cout << "\n=== Testing batch remove Function ===" << std::endl;
    
    Ringbuffer<int> rb;
    
    // Insert 10 elements
    for (int i = 0; i < 10; i++) {
        rb.insert(i);
    }
    
    // Batch delete 5
    size_t removed = rb.remove(5);
    ASSERT_EQ(5, removed, "Batch delete count correct");
    ASSERT_EQ(5, rb.readAvailable(), "Remaining count correct");
}

// Test pointer insert and remove
void test_pointer_insert() {
    std::cout << "\n=== Testing pointer insert/remove Functions ===" << std::endl;
    
    Ringbuffer<int*> rb;
    
    int values[] = {10, 20, 30};
    
    // Insert pointers
    for (int i = 0; i < 3; i++) {
        rb.insert(&values[i]);
    }
    
    // Remove pointers
    for (int i = 0; i < 3; i++) {
        int* val = nullptr;
        rb.remove(val);
        ASSERT_TRUE(val != nullptr, "Removed pointer is valid");
        ASSERT_EQ(values[i], *val, "Pointer points to correct value");
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   Ringbuffer Module Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    test_empty_full();
    test_insert_remove();
    test_peek();
    test_at();
    test_clear();
    test_available();
    test_buff_io();
    test_batch_remove();
    test_pointer_insert();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "   Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "   Passed: " << tests_passed << std::endl;
    std::cout << "   Failed: " << tests_failed << std::endl;
    std::cout << "========================================" << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}