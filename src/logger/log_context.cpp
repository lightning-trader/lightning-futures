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
#include "log_context.h"
#include "nanolog.hpp"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <sstream>
#include <string>
#include <vector>
#include <time_utils.hpp>
#include <string_helper.hpp>
#include <process_helper.hpp>

#ifdef _WIN32
#include <DbgHelp.h>
#include <direct.h>
#pragma comment(lib, "Dbghelp.lib")
#else
#include <execinfo.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

using namespace lt;
using namespace nanolog;

std::atomic < NanoLogger* > atomic_nanologger;

NanoLogger nanologger(atomic_nanologger,"./log",128U);

namespace
{
	thread_local std::string tls_stacktrace;

	std::string capture_stacktrace(uint32_t skip)
	{
#ifdef _WIN32
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, nullptr, TRUE);

		void* frames[62] = {};
		const USHORT captured = CaptureStackBackTrace(static_cast<DWORD>(skip + 1U), static_cast<DWORD>(std::size(frames)), frames, nullptr);
		if (captured == 0U)
		{
			return "stack: <unavailable>";
		}

		std::ostringstream oss;
		oss << "stack:";
		std::vector<char> symbol_buffer(sizeof(SYMBOL_INFO) + MAX_SYM_NAME, 0);
		auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer.data());
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;

		for (USHORT i = 0; i < captured; ++i)
		{
			DWORD64 address = reinterpret_cast<DWORD64>(frames[i]);
			DWORD64 displacement = 0;
			IMAGEHLP_LINE64 line = {};
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
			DWORD line_displacement = 0;

			oss << "\n  #" << i;
			if (SymFromAddr(process, address, &displacement, symbol))
			{
				oss << ' ' << symbol->Name;
			}
			else
			{
				oss << " 0x" << std::hex << address << std::dec;
			}

			if (SymGetLineFromAddr64(process, address, &line_displacement, &line))
			{
				oss << " [" << line.FileName << ':' << line.LineNumber << ']';
			}
		}
		return oss.str();
#else
		void* frames[64] = {};
		const int captured = ::backtrace(frames, static_cast<int>(std::size(frames)));
		if (captured <= 0)
		{
			return "stack: <unavailable>";
		}

		char** symbols = ::backtrace_symbols(frames, captured);
		std::ostringstream oss;
		oss << "stack:";
		for (int i = static_cast<int>(skip) + 1; i < captured; ++i)
		{
			oss << "\n  #" << (i - static_cast<int>(skip) - 1) << ' ';
			if (symbols)
			{
				oss << symbols[i];
			}
			else
			{
				oss << frames[i];
			}
		}
		if (symbols)
		{
			std::free(symbols);
		}
		return oss.str();
#endif
	}

	void log_crash_message(LogLevel level, const char* source, const std::string& message)
	{
		auto* logger = atomic_nanologger.load(std::memory_order_acquire);
		if (!logger)
		{
			return;
		}

		NanoLogLine* line = logger->alloc();
		if (!line)
		{
			return;
		}
		line->initialize(level, source, source, 0U);
		line->append_string(message);
		logger->dump(line);
		logger->flush(2000U);
	}

#ifdef _WIN32
	void ensure_dump_directory()
	{
		::_mkdir(".\\dump");
	}

	void write_minidump(EXCEPTION_POINTERS* exception_pointers)
	{
		ensure_dump_directory();
		auto time_string = lt::datetime_to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()), "%Y-%m-%d_%H%M%S");
		auto file_name = lt::string_helper::format(".\\dump\\lt_{0}.{1}.dmp", time_string, process_helper::get_pid());
		HANDLE file = CreateFileA(file_name.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
		{
			return;
		}

		MINIDUMP_EXCEPTION_INFORMATION dump_info = {};
		dump_info.ThreadId = GetCurrentThreadId();
		dump_info.ExceptionPointers = exception_pointers;
		dump_info.ClientPointers = FALSE;

		MiniDumpWriteDump(GetCurrentProcess(),
			GetCurrentProcessId(),
			file,
			static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithThreadInfo),
			exception_pointers ? &dump_info : nullptr,
			nullptr,
			nullptr);
		CloseHandle(file);
	}

	LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS* exception_pointers)
	{
		log_crash_message(LogLevel::LLV_FATAL, "unhandled_exception_filter", std::string("Unhandled exception captured\n") + capture_stacktrace(1U));
		write_minidump(exception_pointers);
		return EXCEPTION_EXECUTE_HANDLER;
	}
#else
	void enable_core_dump()
	{
		rlimit limit = {};
		if (::getrlimit(RLIMIT_CORE, &limit) != 0)
		{
			return;
		}
		limit.rlim_cur = limit.rlim_max;
		if (limit.rlim_cur == RLIM_INFINITY || limit.rlim_max == RLIM_INFINITY)
		{
			limit.rlim_cur = RLIM_INFINITY;
			limit.rlim_max = RLIM_INFINITY;
		}
		::setrlimit(RLIMIT_CORE, &limit);
	}

	void signal_handler(int signal_number)
	{
		std::ostringstream oss;
		oss << "Signal crash captured: " << signal_number << '\n' << capture_stacktrace(2U);
		log_crash_message(LogLevel::LLV_FATAL, "signal_handler", oss.str());

		std::signal(signal_number, SIG_DFL);
		::raise(signal_number);
	}
#endif

	void terminate_handler()
	{
		std::string reason = "std::terminate triggered";
		if (auto current = std::current_exception())
		{
			try
			{
				std::rethrow_exception(current);
			}
			catch (const std::exception& ex)
			{
				reason.append(": ");
				reason.append(ex.what());
			}
			catch (...)
			{
				reason.append(": unknown exception");
			}
		}

		log_crash_message(LogLevel::LLV_FATAL, "terminate_handler", reason + "\n" + capture_stacktrace(2U));
#ifdef _WIN32
		write_minidump(nullptr);
#endif
		std::abort();
	}

	void install_crash_handlers()
	{
#ifdef _WIN32
		SetUnhandledExceptionFilter(unhandled_exception_filter);
#else
		enable_core_dump();
		std::signal(SIGABRT, signal_handler);
		std::signal(SIGSEGV, signal_handler);
		std::signal(SIGILL, signal_handler);
		std::signal(SIGFPE, signal_handler);
#endif
		std::set_terminate(terminate_handler);
	}

	struct crash_handler_initializer
	{
		crash_handler_initializer()
		{
			install_crash_handlers();
		}
	};

	crash_handler_initializer global_crash_handler_initializer;
}

NanoLogLine* _alloc_logline()
{
	return atomic_nanologger.load(std::memory_order_acquire)->alloc();
}

void _recycle_logline(NanoLogLine* line)
{
	atomic_nanologger.load(std::memory_order_acquire)->recycle(line);
}

void _dump_logline(NanoLogLine* line)
{
	atomic_nanologger.load(std::memory_order_acquire)->dump(line);
}

void _flush_log(uint32_t timeout_ms)
{
	atomic_nanologger.load(std::memory_order_acquire)->flush(timeout_ms);
}

const char* _current_stacktrace(uint32_t skip)
{
	tls_stacktrace = capture_stacktrace(skip + 1U);
	return tls_stacktrace.c_str();
}

void _set_log_option(LogLevel level, uint8_t field, uint8_t print)
{
	atomic_nanologger.load(std::memory_order_acquire)->set_option(level, field, print);
}

uint8_t _get_log_field()
{
	return atomic_nanologger.load(std::memory_order_acquire)->field_mask();
}

