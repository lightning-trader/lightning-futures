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
#include <library_helper.hpp>
#include <mpsc_queue.hpp>
#include <thread>
#include <sstream>

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

struct NanoLogLine
{
public:

	NanoLogLine() = default;

	~NanoLogLine() = default;

	NanoLogLine(NanoLogLine&&) = default;

	NanoLogLine& operator=(NanoLogLine&&) = default;

	std::stringstream _buffer;

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
	}

};

typedef mpsc::mpsc_queue<NanoLogLine> LoglineQueue;



namespace lt::log
{
	class log_wapper
	{
	private:

		dll_handle _handle;

		typedef NanoLogLine* (*ltl_alloc_logline)();

		typedef void (*ltl_recycle_logline)(NanoLogLine*);

		typedef void (*ltl_dump_logline)(NanoLogLine*);

		ltl_alloc_logline _alloc_logline;

		ltl_recycle_logline _recycle_logline;

		ltl_dump_logline _dump_logline;

	public:

		bool is_ready()
		{
			return _alloc_logline && _recycle_logline && _dump_logline;
		}

		log_wapper() :_alloc_logline(nullptr), _recycle_logline(nullptr), _dump_logline(nullptr)
		{
			_handle = library_helper::load_library("liblt-logger-v3x");
			if (_handle)
			{
				_alloc_logline = (ltl_alloc_logline)library_helper::get_symbol(_handle, "ltl_alloc_logline");
				_recycle_logline = (ltl_recycle_logline)library_helper::get_symbol(_handle, "ltl_recycle_logline");
				_dump_logline = (ltl_dump_logline)library_helper::get_symbol(_handle, "ltl_dump_logline");
			}
		}

		virtual ~log_wapper()
		{

			library_helper::free_library(_handle);
		}

		NanoLogLine* alloc_logline()
		{
			if (_alloc_logline)
			{
				return _alloc_logline();
			}
			return nullptr;
		}

		void recycle_logline(NanoLogLine* line)
		{
			if (_recycle_logline)
			{
				_recycle_logline(line);
			}
		}

		void dump_logline(NanoLogLine* line)
		{
			if (_dump_logline)
			{
				_dump_logline(line);
			}
		}
	};

	static log_wapper logger;

	class logline
	{

		NanoLogLine* _line;

		bool _is_dump;

	public:

		logline(LogLevel lv, const char* file, char const* func, uint32_t line) :_is_dump(false), _line(nullptr)
		{
			_line = logger.alloc_logline();
			_line->initialize(lv, file, func, line);
		}

		~logline()
		{
			if (!_is_dump)
			{
				logger.recycle_logline(_line);
			}
		}

		template <typename Frist, typename... Types>
		typename std::enable_if < !std::is_enum <Frist>::value, void >::type
			print(Frist firstArg, Types... args) {
			_line->_buffer << static_cast<std::decay_t<Frist>>(firstArg) << " ";
			print(args...);
		}
		template <typename Frist, typename... Types>
		typename std::enable_if < std::is_enum <Frist>::value, void >::type
			print(Frist firstArg, Types... args) {
			_line->_buffer << static_cast<uint8_t>(firstArg) << " ";
			print(args...);
		}
		template <typename... Types>
		void print(const std::string& firstArg, Types... args) {
			_line->_buffer << firstArg.c_str() << " ";
			print(args...);
		}
		template <typename... Types>
		void print(char* firstArg, Types... args) {
			_line->_buffer << static_cast<const char*>(firstArg) << " ";
			print(args...);
		}
		void print()
		{
			_is_dump = true;
			logger.dump_logline(_line);
		}

	};
}

#ifndef NDEBUG
#define LOG_TRACE(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_TRACE,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define LOG_DEBUG(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_DEBUG,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define PROFILE_DEBUG(msg) //log_profile(LogLevel::LLV_DEBUG,__FILE__,__func__,__LINE__,msg);
#else
#define LOG_TRACE(...) 
#define LOG_DEBUG(...) 
#define PROFILE_DEBUG(msg) 
#endif
#define LOG_INFO(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_INFO,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define LOG_WARNING(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_WARNING,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define LOG_ERROR(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_ERROR,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define LOG_FATAL(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_FATAL,__FILE__,__func__,__LINE__).print(__VA_ARGS__);throw std::runtime_error("log fatal ");

//测评日志
#define EVALUATE_INFO(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_INFO,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
//性能测试
#define PROFILE_INFO(...) //if(logger.is_ready())logline(LogLevel::LLV_INFO,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
