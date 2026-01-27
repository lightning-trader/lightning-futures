/*
Distributed under the MIT License(MIT)

Copyright(c) 2023 Jihua Zou EMail: ghuazo@qq.com QQ:137336521

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in the
Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once
#include <basic_define.h>
#include <iostream>  
#include <vector>  

class memory_pool {
private:
	unsigned char* _data ;
	size_t _chunk_size;
	size_t _block_size;
	size_t _alignment;
	std::vector<bool> _used_state;
	size_t _next_free ;
public:
	memory_pool(size_t chunk_size, size_t alignment) : 
		_chunk_size(chunk_size), 
		_block_size(alignment), 
		_alignment(alignment), 
		_next_free(0)
	{
		if (chunk_size == 0 || alignment == 0) {
			throw std::invalid_argument("chunk_size and alignment must be positive");
		}
		_data = reinterpret_cast<unsigned char*>(malloc(chunk_size * alignment));
		if (!_data) {
			throw std::bad_alloc();
		}
		_used_state.resize(chunk_size, false);
	}
	~memory_pool()
	{
		if (_data) {
			free(_data);
			_data = nullptr;
		}
		_used_state.clear();
	}

	void* allocate(size_t size) {
		if (size == 0) {
			return nullptr;
		}
		if (size > _chunk_size) {
			throw std::bad_alloc();
		}

		// 从_next_free开始搜索
		size_t start = _next_free;
		size_t i = start;
		bool is_finded = false;
		size_t found_pos = 0;

		// 第一次搜索：从_next_free到末尾
		for (; i < _chunk_size; i++) {
			if (!_used_state[i]) {
				// 检查是否有足够的连续空间
				size_t j = 0;
				is_finded = true;
				for (; j < size && (i + j) < _chunk_size; j++) {
					if (_used_state[i + j]) {
						is_finded = false;
						break;
					}
				}
				if (is_finded && j == size) {
					found_pos = i;
					break;
				}
				i += j;
			}
		}

		// 第二次搜索：从开头到start-1
		if (!is_finded) {
			for (i = 0; i < start; i++) {
				if (!_used_state[i]) {
					// 检查是否有足够的连续空间
					size_t j = 0;
					is_finded = true;
					for (; j < size && (i + j) < _chunk_size; j++) {
						if (_used_state[i + j]) {
							is_finded = false;
							break;
						}
					}
					if (is_finded && j == size) {
						found_pos = i;
						break;
					}
					i += j;
				}
			}
		}

		if(!is_finded)
		{
			throw std::bad_alloc();
		}

		// 标记为已使用
		for (size_t k = 0; k < size; k++) {
			_used_state[found_pos + k] = true;
		}

		// 更新_next_free
		_next_free = found_pos + size;
		if (_next_free >= _chunk_size) {
			_next_free = 0;
		}

		// 计算正确的内存地址
		void* ptr = _data + (found_pos * _block_size);
		return ptr;
	}

	void deallocate(void* ptr, size_t size = 0) {
		if (!ptr || !_data || size == 0) {
			return;
		}
		if (size > _chunk_size) {
			return;
		}

		// 计算偏移量
		size_t offset = static_cast<size_t>((unsigned char*)ptr - _data) / _block_size;
		if (offset >= _chunk_size) {
			return;
		}

		// 标记为未使用
		for(size_t i = 0; i < size && (offset + i) < _chunk_size; i++) {
			_used_state[offset + i] = false;
		}

		// 如果释放的内存比当前_next_free早，更新_next_free
		if (offset < _next_free) {
			_next_free = offset;
		}
	}

	inline size_t get_chunk_size()const
	{
		return _chunk_size;
	}

	inline size_t get_block_size()const
	{
		return _block_size;
	}

	inline bool is_valid()const
	{
		return _data != nullptr;
	}

};

template <typename T>
class pool_allocator 
{
public:
	typedef size_t size_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	
	using value_type = T;
	
	template<typename _Tp1>
	struct rebind
	{
		typedef pool_allocator<_Tp1> other;
	};

	pointer allocate(size_type n)
	{
		if (n == 0) {
			return nullptr;
		}
		pointer obj = reinterpret_cast<pointer>(_pool.allocate(n));
		PRINT_DEBUG("Alloc bytes : ", n * sizeof(T));
		return obj;
	}

	void deallocate(pointer obj, size_type n)
	{
		if (obj) {
			_pool.deallocate(obj, n);
			PRINT_DEBUG("Dealloc bytes : ", n * sizeof(T));
		}
	}

	// 默认构造函数，使用合理的默认大小
	pool_allocator() throw() : _pool(1024, sizeof(T))
	{
	}

	// 指定最大大小的构造函数
	pool_allocator(size_t max_size) throw() : _pool(max_size, sizeof(T))
	{
	}

	// 从其他类型的 pool_allocator 构造
	template<class _Tp1>
	pool_allocator(const pool_allocator<_Tp1>& a) throw() : _pool(a.get_cache_size(), sizeof(T))
	{
	}

	// 复制构造函数
	pool_allocator(const pool_allocator<T>& a)  throw() : _pool(a.get_cache_size(), sizeof(T))
	{
	}
	
	// 析构函数
	~pool_allocator() throw()
	{
	}

	// 比较运算符
	bool operator==(const pool_allocator<T>& other) const
	{
		return get_cache_size() == other.get_cache_size();
	}

	bool operator!=(const pool_allocator<T>& other) const
	{
		return !(*this == other);
	}

public:
	
	size_t get_cache_size()const
	{
		return _pool.get_chunk_size();
	}

private:

	memory_pool _pool;
};