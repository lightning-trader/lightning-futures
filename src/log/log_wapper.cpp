#include <log_wapper.hpp>
#include "nanolog.hpp"
#include <chrono>
#include <time_utils.hpp>


bool _is_log_ready = false ;
using namespace nanolog;


bool init_log_environment()
{
	auto& file_name = datetime_to_string(get_now(),"%Y-%m-%d_%H%M%S");
	initialize(GuaranteedLogger(), "./log/","lt_" + file_name, 512);
#ifndef NDEBUG
	uint8_t field = static_cast<uint8_t>(LogField::TIME_SPAMP) | static_cast<uint8_t>(LogField::THREAD_ID) | static_cast<uint8_t>(LogField::LOG_LEVEL) | static_cast<uint8_t>(LogField::SOURCE_FILE);
	uint8_t print = static_cast<uint8_t>(LogPrint::LOG_FILE) | static_cast<uint8_t>(LogPrint::CONSOLE);
	set_log_option(LogLevel::LOG_TRACE, field, print);
#else
	uint8_t field = static_cast<uint8_t>(LogField::TIME_SPAMP) | static_cast<uint8_t>(LogField::THREAD_ID) | static_cast<uint8_t>(LogField::LOG_LEVEL) | static_cast<uint8_t>(LogField::FUNCTION);
	uint8_t print = static_cast<uint8_t>(LogPrint::LOG_FILE) ;
	set_log_option(LogLevel::LOG_INFO, field, print);
#endif
	return true;
}

unsigned char* alloc_log_buffer() 
{
	return alloc_buffer();
}

void free_log_buffer(unsigned char* dataptr)
{
	free_buffer(dataptr);
}

void log_print(log_level lv, const char* file, char const* func, uint32_t line, const unsigned char * msg_data)
{
	if(!_is_log_ready)
	{
		_is_log_ready = init_log_environment();
	}
	if (!is_logged(static_cast<LogLevel>(lv)))
	{
		return;
	}
	add_logline(NanoLogLine(static_cast<LogLevel>(lv), file, func, line, msg_data));
}

void log_profile(log_level lv, const char* file, char const* func, uint32_t line, const char* msg)
{
	unsigned char* buffer = alloc_buffer();
	stream_buffer sd(buffer,1024);
	auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
	static thread_local std::chrono::nanoseconds last_time;
	sd << msg << " now " << now.count() << " use: " << (now - last_time).count();
	log_print(lv, file, func, line, buffer);
	last_time = now;
}