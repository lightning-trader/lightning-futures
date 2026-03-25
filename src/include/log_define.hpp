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
#include <chrono>
#include <stdexcept>
#include <string>

enum class LogLevel : uint8_t
{
	LLV_TRACE = 0U,
	LLV_DEBUG = 1U,
	LLV_INFO = 2U,
	LLV_WARNING = 3U,
	LLV_ERROR = 4U,
	LLV_FATAL = 5U,
};

enum class LogField : uint8_t
{
	TIME_SPAMP = 0B00000001,
	THREAD_ID = 0B00000010,
	LOG_LEVEL = 0B00000100,
	SOURCE_FILE = 0B00001000,
	FUNCTION = 0B00010000,
	STACK_TRACE = 0B00100000,
};

enum class LogPrint : uint8_t
{
	LOG_FILE = 0B00000001,
	CONSOLE = 0B00000010,
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

		typedef NanoLogLine* (*alloc_logline_function)();

		typedef void (*recycle_logline_function)(NanoLogLine*);

		typedef void (*dump_logline_function)(NanoLogLine*);

		typedef void (*flush_log_function)(uint32_t);

		typedef const char* (*current_stacktrace_function)(uint32_t);

		typedef void (*set_log_option_function)(LogLevel, uint8_t, uint8_t);

		typedef uint8_t (*get_log_field_function)();

		alloc_logline_function _alloc_logline;

		recycle_logline_function _recycle_logline;

		dump_logline_function _dump_logline;

		flush_log_function _flush_log;

		current_stacktrace_function _current_stacktrace;

		set_log_option_function _set_log_option;

		get_log_field_function _get_log_field;

	public:

		bool is_ready()
		{
			return _handle && _alloc_logline && _recycle_logline && _dump_logline && _flush_log && _current_stacktrace && _set_log_option && _get_log_field;
		}

		log_wapper() :_alloc_logline(nullptr), _recycle_logline(nullptr), _dump_logline(nullptr), _flush_log(nullptr), _current_stacktrace(nullptr), _set_log_option(nullptr), _get_log_field(nullptr)
		{
#ifndef NOLOG
			_handle = library_helper::load_library("latf-logger-v3x","-d");
			if (_handle)
			{
				_alloc_logline = (alloc_logline_function)library_helper::get_symbol(_handle, "_alloc_logline");
				_recycle_logline = (recycle_logline_function)library_helper::get_symbol(_handle, "_recycle_logline");
				_dump_logline = (dump_logline_function)library_helper::get_symbol(_handle, "_dump_logline");
				_flush_log = (flush_log_function)library_helper::get_symbol(_handle, "_flush_log");
				_current_stacktrace = (current_stacktrace_function)library_helper::get_symbol(_handle, "_current_stacktrace");
				_set_log_option = (set_log_option_function)library_helper::get_symbol(_handle, "_set_log_option");
				_get_log_field = (get_log_field_function)library_helper::get_symbol(_handle, "_get_log_field");
			}
#endif
		}

		virtual ~log_wapper()
		{
#ifndef NOLOG
		if (_handle)
		{
			library_helper::free_library(_handle);
		}
#endif

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

		void flush(uint32_t timeout_ms = 1000U)
		{
			if (_flush_log)
			{
				_flush_log(timeout_ms);
			}
		}

		std::string current_stacktrace(uint32_t skip = 0U)
		{
			if (_current_stacktrace)
			{
				const char* trace = _current_stacktrace(skip);
				if (trace)
				{
					return std::string(trace);
				}
			}
			return {};
		}

		void set_option(LogLevel level, uint8_t field, uint8_t print)
		{
			if (_set_log_option)
			{
				_set_log_option(level, field, print);
			}
		}

		bool has_field(LogField field)
		{
			return _get_log_field &&
				((_get_log_field() & static_cast<uint8_t>(field)) != 0U);
		}
	};

	static log_wapper logger;

	class logline
	{

		NanoLogLine* _line;

		bool _is_dump;

		LogLevel _level;

	public:

		logline(LogLevel lv, const char* file, char const* func, uint32_t line) :_is_dump(false), _level(lv), _line(nullptr)
		{
			_line = logger.alloc_logline();
			if(_line)
			{
				_line->initialize(lv, file, func, line);
			}
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
			if (_line)
			{
				_line->_buffer << static_cast<std::decay_t<Frist>>(firstArg) << " ";
			}
			print(args...);
		}
		template <typename Frist, typename... Types>
		typename std::enable_if < std::is_enum <Frist>::value, void >::type
			print(Frist firstArg, Types... args) {
			if (_line)
			{
				_line->_buffer << static_cast<uint8_t>(firstArg) << " ";
			}
			print(args...);
		}
		template <typename... Types>
		void print(const std::string& firstArg, Types... args) {
			if (_line)
			{
				_line->_buffer << firstArg.c_str() << " ";
			}
			print(args...);
		}
		template <typename... Types>
		void print(char* firstArg, Types... args) {
			if (_line)
			{
				_line->_buffer << static_cast<const char*>(firstArg) << " ";
			}
			print(args...);
		}
		void print()
		{
			if (!_line)
			{
				return;
			}
			_is_dump = true;
			logger.dump_logline(_line);
		}

		template <typename... Types>
		void print_with_stack(Types... args)
		{
			print(args...);
			if (_line && _level >= LogLevel::LLV_ERROR && logger.has_field(LogField::STACK_TRACE))
			{
				auto stack = logger.current_stacktrace(3U);
				if (!stack.empty())
				{
					_line->_buffer << "\n" << stack;
				}
			}
			print();
		}

	};
}

#ifndef NDEBUG
#define PRINT_TRACE(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_TRACE,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define PRINT_DEBUG(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_DEBUG,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define PRINT_FATAL(...) if(lt::log::logger.is_ready()){lt::log::logline(LogLevel::LLV_FATAL,__FILE__,__func__,__LINE__).print_with_stack(__VA_ARGS__);lt::log::logger.flush(2000U);}throw std::runtime_error("log fatal : "+std::string(__func__));
#define PROFILE_DEBUG(...) //log_profile(LogLevel::LLV_DEBUG,__FILE__,__func__,__LINE__,msg);
#else
#define PRINT_TRACE(...) 
#define PRINT_DEBUG(...) 
#define PROFILE_DEBUG(...) 
#define PRINT_FATAL(...) if(lt::log::logger.is_ready()){lt::log::logline(LogLevel::LLV_FATAL,__FILE__,__func__,__LINE__).print_with_stack(__VA_ARGS__);lt::log::logger.flush(2000U);}
#endif
#define PRINT_INFO(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_INFO,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define PRINT_WARNING(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_WARNING,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
#define PRINT_ERROR(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_ERROR,__FILE__,__func__,__LINE__).print_with_stack(__VA_ARGS__);

//测评日志
#define EVALUATE_INFO(...) if(lt::log::logger.is_ready())lt::log::logline(LogLevel::LLV_INFO,__FILE__,__func__,__LINE__).print(__VA_ARGS__);
//性能测试
#define PROFILE_INFO(...) //if(logger.is_ready())logline(LogLevel::LLV_INFO,__FILE__,__func__,__LINE__).print(__VA_ARGS__);


