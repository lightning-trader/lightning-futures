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
#include <define.h>
#include <chrono>
#include <memory>
#include <thread>
#include <mpsc_queue.hpp>
#include <atomic_pool.hpp>
#include <stream_buffer.hpp>


enum class LogLevel : uint8_t
{
	LLV_TRACE = 0U,
	LLV_DEBUG = 1U,
	LLV_INFO = 2U,
	LLV_WARNING = 3U,
	LLV_ERROR = 4U,
	LLV_FATAL = 5U,
};

#define LOG_BUFFER_SIZE 1024U
#ifndef NDEBUG
#define LOG_QUEUE_SIZE 16384U
#else
#define LOG_QUEUE_SIZE 8192U
#endif
struct NanoLogLine
{
public:

	NanoLogLine() = default;

	~NanoLogLine() = default;

	NanoLogLine(NanoLogLine&&) = default;

	NanoLogLine& operator=(NanoLogLine&&) = default;

	unsigned char _buffer[LOG_BUFFER_SIZE] = {0};

	uint64_t _timestamp = 0LLU;

	std::thread::id _thread_id = std::this_thread::get_id();
	
	LogLevel _log_level = LogLevel::LLV_TRACE;
	
	std::string _source_file = "";
	
	std::string _function = "";
	
	uint32_t	_source_line = 0U;

	void initialize(LogLevel lv, const char* file, char const* func, uint32_t line)
	{
		_log_level = lv;
		_source_file = file;
		_function = func;
		_source_line = line;
		_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		_thread_id = std::this_thread::get_id();
		memset(_buffer, 0, LOG_BUFFER_SIZE);
	}

};
typedef atomic_pool<NanoLogLine, LOG_QUEUE_SIZE> LoglinePool;
typedef mpsc::mpsc_queue<NanoLogLine> LoglineQueue;

extern "C"
{
	EXPORT_FLAG void init_log(const char* path, size_t file_size);

	EXPORT_FLAG bool is_ready() ;

	EXPORT_FLAG NanoLogLine* alloc_logline();

	EXPORT_FLAG void recycle_logline(NanoLogLine* line);

	EXPORT_FLAG void dump_logline(NanoLogLine* line);

}



class logline 
{
	std::unique_ptr<stream_carbureter> _sd;
	
	NanoLogLine* _line;

	bool _is_dump ;

public:


	logline(LogLevel lv, const char* file, char const* func, uint32_t line):_is_dump(false), _line(nullptr)
	{
		_line = alloc_logline();
		_line->initialize(lv, file, func, line);
		_sd = std::make_unique<stream_carbureter>(_line->_buffer, LOG_BUFFER_SIZE);
	}

	~logline()
	{
		if(!_is_dump)
		{
			recycle_logline(_line);
		}
	}

	template <typename Frist, typename... Types>
	typename std::enable_if < !std::is_enum <Frist>::value, void >::type
		print(Frist firstArg, Types... args) {
		*_sd << static_cast<std::decay_t<Frist>>(firstArg) << " ";
		print(args...);
	}
	template <typename Frist, typename... Types>
	typename std::enable_if < std::is_enum <Frist>::value, void >::type
		print(Frist firstArg, Types... args) {
		*_sd << static_cast<uint8_t>(firstArg) << " ";
		print(args...);
	}
	template <typename... Types>
	void print(const std::string& firstArg, Types... args) {
		*_sd << firstArg.c_str() << " ";
		print(args...);
	}
	template <typename... Types>
	void print(char* firstArg, Types... args) {
		*_sd << static_cast<const char*>(firstArg) << " ";
		print(args...);
	}
	void print()
	{
		_is_dump = true;
		dump_logline(_line);
	}
	
};


#ifndef NDEBUG
#define LOG_TRACE(...) if(is_ready())logline(LogLevel::LLV_TRACE,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define LOG_DEBUG(...) if(is_ready())logline(LogLevel::LLV_DEBUG,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define PROFILE_DEBUG(msg) //log_profile(LogLevel::LLV_DEBUG,__FILE__,__func__,__LINE__,msg);
#else
#define LOG_TRACE(...) 
#define LOG_DEBUG(...) 
#define PROFILE_DEBUG(msg) 
#endif
#define LOG_INFO(...) if(is_ready())logline(LogLevel::LLV_INFO,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define LOG_WARNING(...) if(is_ready())logline(LogLevel::LLV_WARNING,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define LOG_ERROR(...) if(is_ready())logline(LogLevel::LLV_ERROR,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define LOG_FATAL(...) if(is_ready())logline(LogLevel::LLV_FATAL,__FILE__,__func__,__LINE__).print(__VA_ARGS__);

#define PROFILE_INFO(msg) //log_profile(LLV_INFO,__FILE__,__func__,__LINE__,msg);
