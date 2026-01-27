#include <iostream>
#include <vector>
#include "../src/share/memory_pool.hpp"

// 测试 memory_pool 类
void test_memory_pool() {
    std::cout << "=== 测试 memory_pool 类 ===" << std::endl;
    
    // 测试初始化
    std::cout << "1. 测试初始化:" << std::endl;
    try {
        memory_pool pool(1024, sizeof(int));
        std::cout << "   ✓ memory_pool 初始化成功" << std::endl;
        std::cout << "   chunk_size = " << pool.get_chunk_size() << std::endl;
        std::cout << "   block_size = " << pool.get_block_size() << std::endl;
        std::cout << "   is_valid() = " << pool.is_valid() << std::endl;
        
        // 测试 allocate 方法
        std::cout << "\n2. 测试 allocate 方法:" << std::endl;
        
        // 分配一个块
        void* ptr1 = pool.allocate(1);
        std::cout << "   ✓ 分配 1 个块成功，地址: " << ptr1 << std::endl;
        
        // 分配多个块
        void* ptr2 = pool.allocate(10);
        std::cout << "   ✓ 分配 10 个块成功，地址: " << ptr2 << std::endl;
        
        // 测试 deallocate 方法
        std::cout << "\n3. 测试 deallocate 方法:" << std::endl;
        pool.deallocate(ptr1, 1);
        std::cout << "   ✓ 释放 1 个块成功" << std::endl;
        
        pool.deallocate(ptr2, 10);
        std::cout << "   ✓ 释放 10 个块成功" << std::endl;
        
        // 测试内存重用
        std::cout << "\n4. 测试内存重用:" << std::endl;
        void* ptr3 = pool.allocate(1);
        std::cout << "   ✓ 重新分配 1 个块成功，地址: " << ptr3 << std::endl;
        
        // 测试边界情况
        std::cout << "\n5. 测试边界情况:" << std::endl;
        
        // 分配最大块数
        void* ptr4 = pool.allocate(1024);
        std::cout << "   ✓ 分配 1024 个块成功，地址: " << ptr4 << std::endl;
        
        // 释放最大块数
        pool.deallocate(ptr4, 1024);
        std::cout << "   ✓ 释放 1024 个块成功" << std::endl;
        
        pool.deallocate(ptr3, 1);
        std::cout << "   ✓ 释放所有块成功" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "   ✗ 测试失败: " << e.what() << std::endl;
    }
    
    // 测试异常情况
    std::cout << "\n6. 测试异常情况:" << std::endl;
    
    // 测试无效参数
    try {
        memory_pool pool(0, 4);
        std::cout << "   ✗ 应该抛出异常" << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cout << "   ✓ 正确抛出 invalid_argument 异常: " << e.what() << std::endl;
    }
    
    // 测试分配过大的内存
    try {
        memory_pool pool(10, 4);
        void* ptr = pool.allocate(11);
        std::cout << "   ✗ 应该抛出异常" << std::endl;
    } catch (const std::bad_alloc& e) {
        std::cout << "   ✓ 正确抛出 bad_alloc 异常" << std::endl;
    }
    
    std::cout << "=== memory_pool 测试完成 ===\n" << std::endl;
}

// 测试 pool_allocator 类
void test_pool_allocator() {
    std::cout << "=== 测试 pool_allocator 类 ===" << std::endl;
    
    // 测试默认构造函数
    std::cout << "1. 测试默认构造函数:" << std::endl;
    pool_allocator<int> alloc1;
    std::cout << "   ✓ pool_allocator 初始化成功" << std::endl;
    std::cout << "   cache_size = " << alloc1.get_cache_size() << std::endl;
    
    // 测试指定大小的构造函数
    std::cout << "\n2. 测试指定大小的构造函数:" << std::endl;
    pool_allocator<double> alloc2(512);
    std::cout << "   ✓ pool_allocator 初始化成功" << std::endl;
    std::cout << "   cache_size = " << alloc2.get_cache_size() << std::endl;
    
    // 测试 allocate 和 deallocate 方法
    std::cout << "\n3. 测试 allocate 和 deallocate 方法:" << std::endl;
    
    // 分配单个对象
    int* ptr1 = alloc1.allocate(1);
    std::cout << "   ✓ 分配 1 个 int 成功，地址: " << ptr1 << std::endl;
    *ptr1 = 42;
    std::cout << "   赋值: " << *ptr1 << std::endl;
    
    // 分配多个对象
    double* ptr2 = alloc2.allocate(5);
    std::cout << "   ✓ 分配 5 个 double 成功，地址: " << ptr2 << std::endl;
    for (int i = 0; i < 5; i++) {
        ptr2[i] = i * 1.1;
    }
    std::cout << "   赋值: " << ptr2[0] << ", " << ptr2[1] << ", " << ptr2[2] << "..." << std::endl;
    
    // 释放内存
    alloc1.deallocate(ptr1, 1);
    std::cout << "   ✓ 释放 1 个 int 成功" << std::endl;
    
    alloc2.deallocate(ptr2, 5);
    std::cout << "   ✓ 释放 5 个 double 成功" << std::endl;
    
    // 测试复制构造函数
    std::cout << "\n4. 测试复制构造函数:" << std::endl;
    pool_allocator<int> alloc3(alloc1);
    std::cout << "   ✓ 复制构造函数成功" << std::endl;
    std::cout << "   cache_size = " << alloc3.get_cache_size() << std::endl;
    
    // 测试比较运算符
    std::cout << "\n5. 测试比较运算符:" << std::endl;
    pool_allocator<int> alloc4(1024);
    pool_allocator<int> alloc5(1024);
    pool_allocator<int> alloc6(512);
    
    std::cout << "   alloc4 == alloc5: " << (alloc4 == alloc5) << std::endl;
    std::cout << "   alloc4 == alloc6: " << (alloc4 == alloc6) << std::endl;
    std::cout << "   alloc4 != alloc6: " << (alloc4 != alloc6) << std::endl;
    
    // 测试与 STL 容器配合使用
    std::cout << "\n6. 测试与 STL 容器配合使用:" << std::endl;
    try {
        std::vector<int, pool_allocator<int>> vec;
        for (int i = 0; i < 100; i++) {
            vec.push_back(i);
        }
        std::cout << "   ✓ 向 vector 添加 100 个元素成功" << std::endl;
        std::cout << "   vector size: " << vec.size() << std::endl;
        std::cout << "   vector[0]: " << vec[0] << std::endl;
        std::cout << "   vector[99]: " << vec[99] << std::endl;
    } catch (const std::exception& e) {
        std::cout << "   ✗ 测试失败: " << e.what() << std::endl;
    }
    
    std::cout << "=== pool_allocator 测试完成 ===\n" << std::endl;
}

int main() {
    std::cout << "=== 开始 memory_pool 模块单元测试 ===\n" << std::endl;
    
    test_memory_pool();
    test_pool_allocator();
    
    std::cout << "=== memory_pool 模块单元测试完成 ===" << std::endl;
    return 0;
}
