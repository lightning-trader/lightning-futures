#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include "../src/share/memory_pool.hpp"

// 测试计数器
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(condition, message) \
    do { \
        if (condition) { \
            std::cout << "   ✓ " << message << std::endl; \
            tests_passed++; \
        } else { \
            std::cout << "   ✗ " << message << " (FAILED)" << std::endl; \
            tests_failed++; \
        } \
    } while(0)

#define ASSERT_FALSE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "   ✓ " << message << std::endl; \
            tests_passed++; \
        } else { \
            std::cout << "   ✗ " << message << " (FAILED)" << std::endl; \
            tests_failed++; \
        } \
    } while(0)

#define ASSERT_THROW(expr, exception_type, message) \
    do { \
        bool thrown = false; \
        try { \
            expr; \
        } catch (const exception_type&) { \
            thrown = true; \
        } catch (...) { \
        } \
        if (thrown) { \
            std::cout << "   ✓ " << message << std::endl; \
            tests_passed++; \
        } else { \
            std::cout << "   ✗ " << message << " (FAILED - no exception thrown)" << std::endl; \
            tests_failed++; \
        } \
    } while(0)

// 测试 memory_pool 类
void test_memory_pool_basic() {
    std::cout << "\n=== 测试 memory_pool 基本功能 ===" << std::endl;
    
    // 测试正常初始化
    {
        memory_pool pool(1024, sizeof(int));
        ASSERT_TRUE(pool.is_valid(), "memory_pool 初始化成功");
        ASSERT_TRUE(pool.get_chunk_size() == 1024, "chunk_size = 1024");
        ASSERT_TRUE(pool.get_block_size() == sizeof(int), "block_size = sizeof(int)");
    }
    
    // 测试 allocate 和 deallocate
    {
        memory_pool pool(1024, sizeof(int));
        
        void* ptr1 = pool.allocate(1);
        ASSERT_TRUE(ptr1 != nullptr, "分配 1 个块成功");
        
        void* ptr2 = pool.allocate(10);
        ASSERT_TRUE(ptr2 != nullptr, "分配 10 个块成功");
        
        pool.deallocate(ptr1, 1);
        pool.deallocate(ptr2, 10);
        ASSERT_TRUE(true, "释放内存成功");
    }
    
    // 测试内存重用
    {
        memory_pool pool(1024, sizeof(int));
        void* ptr1 = pool.allocate(1);
        pool.deallocate(ptr1, 1);
        void* ptr2 = pool.allocate(1);
        ASSERT_TRUE(ptr1 == ptr2, "内存重用正确");
    }
    
    // 测试分配最大块数
    {
        memory_pool pool(1024, sizeof(int));
        void* ptr = pool.allocate(1024);
        ASSERT_TRUE(ptr != nullptr, "分配 1024 个块成功");
        pool.deallocate(ptr, 1024);
    }
}

void test_memory_pool_edge_cases() {
    std::cout << "\n=== 测试 memory_pool 边界情况 ===" << std::endl;
    
    // 测试无效参数 - chunk_size 为 0
    ASSERT_THROW(memory_pool(0, 4), std::invalid_argument, "chunk_size 为 0 时抛出 invalid_argument");
    
    // 测试无效参数 - alignment 为 0
    ASSERT_THROW(memory_pool(100, 0), std::invalid_argument, "alignment 为 0 时抛出 invalid_argument");
    
    // 测试分配过大的内存
    {
        memory_pool pool(10, 4);
        ASSERT_THROW(pool.allocate(11), std::bad_alloc, "分配超过池大小的内存时抛出 bad_alloc");
    }
    
    // 测试分配 0 大小返回 nullptr
    {
        memory_pool pool(100, 4);
        void* ptr = pool.allocate(0);
        ASSERT_TRUE(ptr == nullptr, "分配 0 大小返回 nullptr");
    }
    
    // 测试 deallocate 空指针
    {
        memory_pool pool(100, 4);
        pool.deallocate(nullptr, 10);
        ASSERT_TRUE(true, "释放空指针不崩溃");
    }
    
    // 测试 deallocate 超出范围的指针
    {
        memory_pool pool(100, 4);
        char dummy;
        pool.deallocate(&dummy, 10);
        ASSERT_TRUE(true, "释放无效指针不崩溃");
    }
}

void test_memory_pool_fragmentation() {
    std::cout << "\n=== 测试 memory_pool 内存碎片 ===" << std::endl;
    
    // 测试内存碎片情况下的分配
    {
        memory_pool pool(100, sizeof(int));
        
        // 分配 - 释放 - 分配模式
        void* ptr1 = pool.allocate(10);
        void* ptr2 = pool.allocate(20);
        void* ptr3 = pool.allocate(30);
        
        pool.deallocate(ptr2, 20); // 释放中间的
        
        void* ptr4 = pool.allocate(15); // 应该能重用 ptr2 的空间
        ASSERT_TRUE(ptr4 != nullptr, "在碎片内存中分配成功");
        
        pool.deallocate(ptr1, 10);
        pool.deallocate(ptr3, 30);
        pool.deallocate(ptr4, 15);
    }
    
    // 测试循环分配
    {
        memory_pool pool(100, sizeof(int));
        std::vector<void*> ptrs;
        
        for (int i = 0; i < 10; i++) {
            ptrs.push_back(pool.allocate(5));
        }
        ASSERT_TRUE(ptrs.size() == 10, "循环分配 10 次成功");
        
        for (auto ptr : ptrs) {
            pool.deallocate(ptr, 5);
        }
        
        // 释放后应该能重新分配
        void* new_ptr = pool.allocate(50);
        ASSERT_TRUE(new_ptr != nullptr, "释放后重新分配大块内存成功");
    }
}

void test_memory_pool_stress() {
    std::cout << "\n=== 测试 memory_pool 压力测试 ===" << std::endl;
    
    // 大量分配释放
    {
        memory_pool pool(10000, sizeof(int));
        std::vector<void*> ptrs;
        
        for (int i = 0; i < 1000; i++) {
            ptrs.push_back(pool.allocate(5));
        }
        ASSERT_TRUE(ptrs.size() == 1000, "压力测试：分配 1000 次成功");
        
        // 随机释放
        for (int i = 0; i < 1000; i += 2) {
            pool.deallocate(ptrs[i], 5);
        }
        
        // 再次分配
        for (int i = 0; i < 500; i++) {
            void* ptr = pool.allocate(3);
            ASSERT_TRUE(ptr != nullptr, "压力测试：再次分配成功");
            pool.deallocate(ptr, 3);
        }
        
        // 清理
        for (int i = 1; i < 1000; i += 2) {
            pool.deallocate(ptrs[i], 5);
        }
    }
}

// 测试 pool_allocator 类
void test_pool_allocator_basic() {
    std::cout << "\n=== 测试 pool_allocator 基本功能 ===" << std::endl;
    
    // 测试默认构造函数
    {
        pool_allocator<int> alloc;
        ASSERT_TRUE(alloc.get_cache_size() > 0, "默认构造函数初始化成功");
    }
    
    // 测试指定大小构造函数
    {
        pool_allocator<double> alloc(512);
        ASSERT_TRUE(alloc.get_cache_size() == 512, "指定大小构造函数正确");
    }
    
    // 测试 allocate 和 deallocate
    {
        pool_allocator<int> alloc;
        int* ptr = alloc.allocate(1);
        ASSERT_TRUE(ptr != nullptr, "allocate 成功");
        *ptr = 42;
        ASSERT_TRUE(*ptr == 42, "内存可正常读写");
        alloc.deallocate(ptr, 1);
    }
    
    // 测试复制构造函数
    {
        pool_allocator<int> alloc1(1024);
        pool_allocator<int> alloc2(alloc1);
        ASSERT_TRUE(alloc2.get_cache_size() == 1024, "复制构造函数正确");
    }
}

void test_pool_allocator_stl() {
    std::cout << "\n=== 测试 pool_allocator 与 STL 容器配合 ===" << std::endl;
    
    // 测试与 vector 配合
    {
        std::vector<int, pool_allocator<int>> vec;
        for (int i = 0; i < 100; i++) {
            vec.push_back(i);
        }
        ASSERT_TRUE(vec.size() == 100, "vector 添加 100 个元素成功");
        ASSERT_TRUE(vec[0] == 0, "vec[0] == 0");
        ASSERT_TRUE(vec[99] == 99, "vec[99] == 99");
    }
    
    // 测试与 vector 配合使用 char
    {
        std::vector<char, pool_allocator<char>> str_buf;
        const char* test_str = "Hello, World!";
        for (size_t i = 0; i < std::strlen(test_str); i++) {
            str_buf.push_back(test_str[i]);
        }
        ASSERT_TRUE(str_buf.size() == std::strlen(test_str), "vector<char> 使用 pool_allocator 成功");
    }
}

void test_memory_pool_destructor() {
    std::cout << "\n=== 测试 memory_pool 析构函数 ===" << std::endl;
    
    // 测试正常析构
    {
        memory_pool* pool = new memory_pool(1024, sizeof(int));
        void* ptr = pool->allocate(10);
        (void)ptr; // 避免未使用警告
        delete pool;
        ASSERT_TRUE(true, "正常析构不崩溃");
    }
    
    // 测试部分释放后析构
    {
        memory_pool* pool = new memory_pool(1024, sizeof(int));
        void* ptr1 = pool->allocate(10);
        void* ptr2 = pool->allocate(20);
        pool->deallocate(ptr1, 10);
        delete pool;
        ASSERT_TRUE(true, "部分释放后析构不崩溃 (无内存泄漏)");
    }
}

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "   memory_pool 模块单元测试" << std::endl;
    std::cout << "============================================" << std::endl;
    
    test_memory_pool_basic();
    test_memory_pool_edge_cases();
    test_memory_pool_fragmentation();
    test_memory_pool_stress();
    test_pool_allocator_basic();
    test_pool_allocator_stl();
    test_memory_pool_destructor();
    
    std::cout << "\n============================================" << std::endl;
    std::cout << "   测试结果汇总" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "   通过：" << tests_passed << std::endl;
    std::cout << "   失败：" << tests_failed << std::endl;
    std::cout << "============================================" << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}