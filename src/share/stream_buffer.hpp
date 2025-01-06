﻿/*
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

#include <cstdint>
#include <memory>
#include <string>
#include <iosfwd>
#include <tuple>
#include <utility>
#include <iostream>
#include <type_traits>


typedef std::tuple <bool, char, unsigned char, short, unsigned short, int, unsigned int, long long, unsigned long long, float, double, const char*> type_table;

template < typename T, typename Tuple >
struct type_index 
{
	static constexpr const std::size_t value = 0;
};

template < typename T, typename ... Types >
struct type_index < T, std::tuple < T, Types... > >
{
	static constexpr const std::size_t value = 0;
};

template < typename T, typename U, typename ... Types >
struct type_index < T, std::tuple < U, Types... > >
{
	static constexpr const std::size_t value = 1 + type_index < T, std::tuple < Types... > >::value;
};

class stream_carbureter
{

private:
	
	size_t _used;
	size_t _max_size;
	unsigned char* _buffer ;


public:

	stream_carbureter(unsigned char* buffer,size_t max_size):_buffer(buffer), _used(1), _max_size(max_size)
	{
		//memset(_buffer, 0, max_size);
		//int8_t len = _buffer[0]; 开头的8位存元素个数
	}

	~stream_carbureter()=default;

	void clear()
	{
		_used = 1;
		memset(_buffer, 0, _max_size);
		_buffer[0] = 0;
	}

public:
	// encode
	template < typename Arg >
	stream_carbureter& operator<<(Arg arg)
	{
		if(_used+sizeof(arg)+1> _max_size)
		{
			throw std::out_of_range("buffer full");
		}
		auto type_id = type_index<Arg, type_table>::value;
		*reinterpret_cast<uint8_t*>(_buffer+_used) = static_cast<uint8_t>(type_id);
		_used++;
		*reinterpret_cast<Arg*>(_buffer+_used) = arg ;
		_used+=sizeof(arg);
		_buffer[0]++;
		return *this;
	}
	
	stream_carbureter& operator<<(const char* arg)
	{
		if (_used + strlen(arg) + 1 + 1 > _max_size)
		{
			throw std::out_of_range("buffer full");
		}
		auto type_id = type_index<const char*, type_table>::value;
		*reinterpret_cast<uint8_t*>(_buffer + _used) = static_cast<uint8_t>(type_id);
		_used++;
		std::memcpy(reinterpret_cast<char*>(_buffer + _used), (arg), strlen(arg));
		_used += strlen(arg);
		*reinterpret_cast<char*>(_buffer + _used) = '\0';
		_used++;
		_buffer[0]++;
		return *this;
	}
	
};
class stream_extractor
{

private:
	size_t _freed;
	size_t _max_size;
	unsigned char* _buffer;


public:

	stream_extractor(unsigned char* buffer, size_t max_size) :_buffer(buffer), _freed(1), _max_size(max_size)
	{
		//memset(_buffer, 0, max_size);
		//int8_t len = _buffer[0]; 开头的8位存元素个数
	}

	~stream_extractor() = default;

	void reset()
	{
		_freed = 1;
	}

public:
	
private:

	template<typename T>
	void extract(std::ostream& os, T* dataptr)
	{
		os << *dataptr;
		_freed += sizeof(T);
	}


	//template <>
	void extract(std::ostream& os, const char* strptr)
	{
		/*
		const char* b = strptr;
		while (*b != '\0')
		{
			os << *b;
			++b;
		}*/
		os << strptr;
		//os << " ";
		_freed += strlen(strptr) + 1;
	}

public:
	// decode
	void out(std::ostream& os)
	{
		uint8_t size = _buffer[0];

		for (uint8_t i = 0; i < size; ++i)
		{
			const int32_t type_id = *reinterpret_cast<uint8_t*>(_buffer + _freed++);
			switch (type_id)
			{
			case 0:
				extract(os, reinterpret_cast<std::tuple_element<0, type_table>::type*>(_buffer + _freed));
				break;
			case 1:
				extract(os, reinterpret_cast<std::tuple_element<1, type_table>::type*>(_buffer + _freed));
				break;
			case 2:
				extract(os, reinterpret_cast<std::tuple_element<2, type_table>::type*>(_buffer + _freed));
				break;
			case 3:
				extract(os, reinterpret_cast<std::tuple_element<3, type_table>::type*>(_buffer + _freed));
				break;
			case 4:
				extract(os, reinterpret_cast<std::tuple_element<4, type_table>::type*>(_buffer + _freed));
				break;
			case 5:
				extract(os, reinterpret_cast<std::tuple_element<5, type_table>::type*>(_buffer + _freed));
				break;
			case 6:
				extract(os, reinterpret_cast<std::tuple_element<6, type_table>::type*>(_buffer + _freed));
				break;
			case 7:
				extract(os, reinterpret_cast<std::tuple_element<7, type_table>::type*>(_buffer + _freed));
				break;
			case 8:
				extract(os, reinterpret_cast<std::tuple_element<8, type_table>::type*>(_buffer + _freed));
				break;
			case 9:
				extract(os, reinterpret_cast<std::tuple_element<9, type_table>::type*>(_buffer + _freed));
				break;
			case 10:
				extract(os, reinterpret_cast<std::tuple_element<10, type_table>::type*>(_buffer + _freed));
				break;
			case 11:
				extract(os, reinterpret_cast<std::tuple_element<11, type_table>::type>(_buffer + _freed));
				break;
			}
		}
	}
};
