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
#include <log_wapper.hpp>
#include "nanolog.hpp"
#include <chrono>
#include <time_utils.hpp>
#include <string_helper.hpp>
#include <process_helper.hpp>
#include <filesystem>


bool _is_log_ready = false ;
using namespace nanolog;

std::unique_ptr < NanoLogger > nanologger;

std::atomic < NanoLogger* > atomic_nanologger;

std::atomic <bool> _is_ready = false ;

bool is_ready()
{
	return _is_ready.load();
}

NanoLogLine* alloc_logline()
{
	return atomic_nanologger.load(std::memory_order_acquire)->alloc();
}

void recycle_logline(NanoLogLine* line)
{
	atomic_nanologger.load(std::memory_order_acquire)->recycle(line);
}

void dump_logline(NanoLogLine* line)
{
	atomic_nanologger.load(std::memory_order_acquire)->dump(line);
}


void init_log(const char* path,size_t file_size)
{
	if (!std::filesystem::exists(path))
	{
		std::filesystem::create_directories(path);
	}
	auto time_string = datetime_to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),"%Y-%m-%d_%H%M%S");
	auto file_name = string_helper::format("lt_{0}.{1}", time_string, process_helper::get_pid());

#ifndef NDEBUG
	nanologger.reset(new NanoLogger(path, file_name, file_size, 10240));
	uint8_t field = static_cast<uint8_t>(LogField::TIME_SPAMP) | static_cast<uint8_t>(LogField::THREAD_ID) | static_cast<uint8_t>(LogField::LOG_LEVEL) | static_cast<uint8_t>(LogField::SOURCE_FILE);
	uint8_t print = static_cast<uint8_t>(LogPrint::LOG_FILE) | static_cast<uint8_t>(LogPrint::CONSOLE);
	nanologger->set_option(LogLevel::LLV_TRACE, field, print);
#else
	nanologger.reset(new NanoLogger(path, file_name, file_size, 1024));
	uint8_t field = static_cast<uint8_t>(LogField::TIME_SPAMP) | static_cast<uint8_t>(LogField::THREAD_ID) | static_cast<uint8_t>(LogField::LOG_LEVEL) ;
	uint8_t print = static_cast<uint8_t>(LogPrint::LOG_FILE) ;
	nanologger->set_option(LogLevel::LLV_INFO, field, print);
#endif
	atomic_nanologger.store(nanologger.get(), std::memory_order_seq_cst);
	_is_ready.store(true, std::memory_order_release);

}
