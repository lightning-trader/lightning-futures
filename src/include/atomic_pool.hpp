#pragma once
#include <define.h>
#include <iostream>  
#include <vector>  

template<typename T,size_t N>
class atomic_pool 
{

private:

	std::atomic<bool> _lock;

	std::vector<T*> _data;

	std::array<std::atomic<bool>,N> _used;

public:

	atomic_pool(size_t init_size) : _lock(false)
	{
		static_assert(std::is_default_constructible<T>::value,
			"atomic_pool requires default constructor");
		for (size_t i = 0; i < N; i++)
		{
			if(i < init_size)
			{
				_data.emplace_back(new T());
			}
			_used[i].store(false, std::memory_order_release);
		}
	}
	~atomic_pool()
	{
		for (size_t i = 0; i < _data.size(); i++)
		{
			delete _data[i];
		}
		_data.clear();
	}

	T* resize(bool is_used)
	{
		while (_lock.exchange(true, std::memory_order_acquire));
		T* ret = nullptr;
		if(_data.size() < N)
		{
			size_t i = _data.size();
			_data.emplace_back(new T());
			_used[i].store(is_used, std::memory_order_release);;
			ret = _data.back();
		}
		_lock.store(false, std::memory_order_release);
		return ret;
	}

	T* alloc(bool force)
	{
		T* ret = nullptr ;
		for (size_t i = 0; i < _data.size(); i++)
		{
			auto& us = _used[i];
			bool expected = false ;
			if(us.compare_exchange_weak(expected,true,std::memory_order_relaxed))
			{
				ret = _data[i];
				break;
			}
		}
		if(ret == nullptr)
		{
			ret = resize(true);
			if(force&&ret == nullptr)
			{
				for (size_t i = 0; ; i = (i+1) % _data.size())
				{
					auto& us = _used[i];
					bool expected = false;
					if (us.compare_exchange_weak(expected, true, std::memory_order_relaxed))
					{
						ret = _data[i];
						break;
					}
				}
			}
		}
		//ret->T();
		return ret;
	}

	void recycle(T* data) 
	{
		for (size_t i = 0; i < _data.size(); i++)
		{
			if( _data[i] == data )
			{
				data->~T();
				_used[i].store(false, std::memory_order_release);
				break;
			}
		}
	}

};
