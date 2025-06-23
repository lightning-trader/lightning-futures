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
	size_t _alignment;
	std::vector<bool> _used_state;
	size_t _iter ;
public:
	memory_pool(size_t chunk_size, size_t alignment) : _chunk_size(chunk_size), _alignment(alignment), _iter(0)
	{
		_data = reinterpret_cast<unsigned char*>(malloc(chunk_size * alignment));
		for(size_t i = 0;i< _chunk_size;i++)
		{
			_used_state.emplace_back(false);
		}
	}
	~memory_pool()
	{
		free(_data);
		_used_state.clear();
	}


	void* allocate(size_t size) {
		if (size > _chunk_size) {
			throw std::bad_alloc();
		}
		bool is_finded = false;
		size_t i = 0;
		while(i < _chunk_size)
		{
			size_t j = 0;
			is_finded = true ;
			for (; j < size; j++)
			{
				if (_used_state[_iter+i+j])
				{
					is_finded = false ;
					break;
				}
			}
			if(is_finded)
			{
				for (size_t k = 0; k < size; k++)
				{
					_used_state[_iter+i+k] = true;
				}
				break;
			}
			i = (i + j + 1) > _chunk_size ? 0 : i + j + 1;
		}
		if(!is_finded)
		{
			throw std::bad_alloc();
		}
		void* ptr = (_data + _iter + i * _alignment);
		_iter = (_iter + size);
		return ptr;
	}

	void deallocate(void* ptr, size_t size = 0) {
		if (!ptr) {
			return;
		}
		if (size > _chunk_size) {
			return;
		}
		size_t offset = static_cast<size_t>((unsigned char*)ptr-_data) / _alignment;
		for(size_t i=0;i<size;i++)
		{
			_used_state[offset + i] = false ;
		}
	}

	inline size_t get_chunk_size()const
	{
		return _chunk_size;
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
		pointer obj = reinterpret_cast<pointer>(_pool.allocate(n));
		PRINT_DEBUG("Alloc bytes : ", n * sizeof(T));
		return obj;
	}

	void deallocate(pointer obj, size_type n)
	{
		_pool.deallocate(obj, n);
		PRINT_DEBUG("Dealloc bytes : ", n * sizeof(T));
	}

	pool_allocator() throw() :_pool(0,sizeof(T))
	{ 
	}

	pool_allocator(size_t max_size) throw() : _pool(max_size, sizeof(T))
	{
	}

	template<class _Tp1>
	pool_allocator(const pool_allocator<_Tp1>& a) throw() : _pool(a.get_cache_size(), sizeof(T))
	{

	}

	pool_allocator(const pool_allocator<T>& a)  throw() : _pool(a.get_cache_size(), sizeof(T))
	{
		
	}
	

	~pool_allocator() throw()
	{
		
	}

public:
	
	size_t get_cache_size()const
	{
		return _pool.get_chunk_size();
	}

private:

	memory_pool _pool;
};