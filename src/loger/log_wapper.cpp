#include <log_wapper.hpp>
#include "nanolog.hpp"
#include <chrono>
#include <time_utils.hpp>
#include <string_helper.hpp>
#include <process_helper.hpp>


bool _is_log_ready = false ;
using namespace nanolog;


bool init_log_environment()
{
	auto time_string = datetime_to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),"%Y-%m-%d_%H%M%S");
	auto file_name = string_helper::format("lt_{0}.{1}", time_string, process_helper::get_pid());
	initialize(GuaranteedLogger(), "./log/", file_name, 128);
#ifndef NDEBUG
	uint8_t field = static_cast<uint8_t>(LogField::TIME_SPAMP) | static_cast<uint8_t>(LogField::THREAD_ID) | static_cast<uint8_t>(LogField::LOG_LEVEL) | static_cast<uint8_t>(LogField::SOURCE_FILE);
	uint8_t print = static_cast<uint8_t>(LogPrint::LOG_FILE) | static_cast<uint8_t>(LogPrint::CONSOLE);
	set_log_option(LogLevel::LOG_TRACE, field, print);
#else
	uint8_t field = static_cast<uint8_t>(LogField::TIME_SPAMP) | static_cast<uint8_t>(LogField::THREAD_ID) | static_cast<uint8_t>(LogField::LOG_LEVEL) ;
	uint8_t print = static_cast<uint8_t>(LogPrint::LOG_FILE) ;
	set_log_option(LogLevel::LOG_INFO, field, print);
#endif
	return true;
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
	unsigned char buffer[LOG_BUFFER_SIZE] = {0};
	stream_carbureter sd(buffer, LOG_BUFFER_SIZE);
	auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
	static thread_local std::chrono::nanoseconds last_time;
	sd << msg << " now " << now.count() << " use: " << (now - last_time).count();
	log_print(lv, file, func, line, buffer);
	last_time = now;
}