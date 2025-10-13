/*

Distributed under the MIT License (MIT)

	Copyright (c) 2016 Karthik Iyengar; Jihua Zou

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in the
Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "nanolog.hpp"
#include <mpsc_queue.hpp>
#include <cstring>
#include <chrono>
#include <ctime>
#include <atomic>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <time_utils.hpp>
#include <process_helper.hpp>
#include <string_helper.hpp>

namespace
{


	/* I want [2016-10-13 00:01:23.528514] */
	void format_timestamp(std::ostream& os, uint64_t timestamp)
	{
		// The next 3 lines do not work on MSVC!
		auto duration = std::chrono::microseconds(timestamp);
		std::chrono::system_clock::time_point time_point(duration);
		std::time_t time_t = std::chrono::system_clock::to_time_t(time_point);
		//std::time_t time_t = timestamp / 1000000;
		auto time = std::localtime(&time_t);
		char buffer[32]={0};
		std::strftime(buffer, 32, "%Y-%m-%d %T.", time);
		char microseconds[7]={0};

#if defined(_MSC_VER)		
		snprintf(microseconds, 7 , "%06I64u", timestamp % 1000000);
#else
#	ifdef __x86_64__
		snprintf(microseconds, 7, "%06lu", timestamp % 1000000);
#	elif __i386__
		snprintf(microseconds, 7, "%06llu", timestamp % 1000000);
#	endif
#endif

		os << '[' << buffer << microseconds << ']';
	}


} // anonymous namespace

namespace nanolog
{
	
	char const* level_to_string(LogLevel level)
	{

		switch (level)
		{
		case LogLevel::LLV_TRACE:
			return "TRACE";
		case LogLevel::LLV_DEBUG:
			return "DEBUG";
		case LogLevel::LLV_INFO:
			return "INFO";
		case LogLevel::LLV_WARNING:
			return "WARNING";
		case LogLevel::LLV_ERROR:
			return "ERROR";
		case LogLevel::LLV_FATAL:
			return "FATAL";
		}
		return "XXXX";
	}


	void logline_stringify(std::ostream& os, const NanoLogLine& logline,uint8_t field)
	{
		if (field & static_cast<uint8_t>(LogField::TIME_SPAMP))
		{
			format_timestamp(os, logline._timestamp);
		}
		if (field & static_cast<uint8_t>(LogField::THREAD_ID))
		{
			os << '[' << logline._thread_id << ']';
		}
		if (field & static_cast<uint8_t>(LogField::LOG_LEVEL))
		{
			os << '[' << level_to_string(logline._log_level) << ']';
		}
		if (field & static_cast<uint8_t>(LogField::SOURCE_FILE))
		{
			os << '[' << logline._source_file << ':' << logline._source_line << "] ";
		}
		if (field & static_cast<uint8_t>(LogField::FUNCTION))
		{
			os << '[' << logline._function << ':' << logline._source_line << "] ";
		}
		os << logline._buffer.str() << std::endl;
		//os.flush();
	}

	class ConsoleWriter : public LogWriter
	{
	
	public:

		ConsoleWriter()
		{
			std::ios::sync_with_stdio(false);
		}

		virtual void write(const NanoLogLine& logline, uint8_t field)
		{
			logline_stringify(std::cout, logline, field);
		}

		~ConsoleWriter()
		{
			std::ios::sync_with_stdio(true);
			std::cout.flush();
		}
	};

	class FileWriter : public LogWriter
	{
	public:
		FileWriter(std::string const& log_directory, std::string const& log_file_name, uint32_t log_file_roll_size_mb)
			: m_log_file_roll_size_bytes(log_file_roll_size_mb * 1024 * 1024)
			, m_name(log_directory + "/" + log_file_name)
		{
			roll_file();
		}
		~FileWriter()
		{
			if (m_os)
			{
				m_os->flush();
				m_os->close();
			}
		}
		virtual void write(const NanoLogLine& logline,uint8_t field)
		{
			auto pos = m_os->tellp();
			logline_stringify(*m_os, logline, field);
			m_bytes_written += m_os->tellp() - pos;
			if (m_bytes_written > m_log_file_roll_size_bytes)
			{
				roll_file();
			}
		}

	private:
		void roll_file()
		{
			if (m_os)
			{
				m_os->flush();
				m_os->close();
			}

			m_bytes_written = 0;
			m_os.reset(new std::ofstream());
			// TODO Optimize this part. Does it even matter ?
			std::string log_file_name = m_name;
			log_file_name.append(".");
			log_file_name.append(std::to_string(++m_file_number));
			log_file_name.append(".txt");
			m_os->open(log_file_name, std::ofstream::out | std::ofstream::trunc);
		}

	private:
		uint32_t m_file_number = 0;
		std::streamoff m_bytes_written = 0;
		uint32_t const m_log_file_roll_size_bytes;
		std::string const m_name;
		std::unique_ptr < std::ofstream > m_os;
	};

	NanoLogger::NanoLogger(std::atomic<NanoLogger*>& holder, std::string const& log_directory, uint32_t file_size_mb)
		: _file_writer(nullptr)
		, _console_writer(nullptr)
		, _is_runing(false)
		, _mq()
		, _level(LogLevel::LLV_TRACE)
		, _field(0)
		, _print(0)
	{


		if (!std::filesystem::exists(log_directory))
		{
			std::filesystem::create_directories(log_directory);
		}
		auto time_string = lt::datetime_to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()), "%Y-%m-%d_%H%M%S");
		auto file_name = lt::string_helper::format("lt_{0}.{1}", time_string, process_helper::get_pid());
		
#ifndef NDEBUG
		uint8_t field = static_cast<uint8_t>(LogField::TIME_SPAMP) | static_cast<uint8_t>(LogField::THREAD_ID) | static_cast<uint8_t>(LogField::LOG_LEVEL) | static_cast<uint8_t>(LogField::SOURCE_FILE);
		uint8_t print = static_cast<uint8_t>(LogPrint::LOG_FILE) | static_cast<uint8_t>(LogPrint::CONSOLE);
		this->set_option(LogLevel::LLV_TRACE, field, print);
#else
		uint8_t field = static_cast<uint8_t>(LogField::TIME_SPAMP) | static_cast<uint8_t>(LogField::THREAD_ID) | static_cast<uint8_t>(LogField::LOG_LEVEL);
		uint8_t print = static_cast<uint8_t>(LogPrint::LOG_FILE);
		this->set_option(LogLevel::LLV_INFO, field, print);
#endif
		holder.store(this, std::memory_order_seq_cst);

		_file_writer = std::make_unique<FileWriter>(log_directory, file_name, std::max<uint32_t>(1u, file_size_mb));
		_console_writer = std::make_unique<ConsoleWriter>();
	
		_is_runing.store(true, std::memory_order_release);
		_thread = std::make_unique<std::thread>(&NanoLogger::pop,this);
	}

	NanoLogger::~NanoLogger()
	{
		if(_thread)
		{
			_is_runing.store(false, std::memory_order_release);
			_thread->join();
		}
	}



	void NanoLogger::pop()
	{

		
		while (_is_runing || !_mq.is_empty())
		{
			NanoLogLine* logline = _mq.pop();
			if (logline)
			{
				if (_print & static_cast<uint8_t>(LogPrint::LOG_FILE))
				{
					_file_writer->write(*logline, _field);
				}
				if (_print & static_cast<uint8_t>(LogPrint::CONSOLE))
				{
					_console_writer->write(*logline, _field);
				}
				recycle(logline);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		
	}



	NanoLogLine* NanoLogger::alloc()
	{
		return new NanoLogLine();
	}

	void NanoLogger::recycle(NanoLogLine* line)
	{
		delete line;
	}

	void NanoLogger::dump(NanoLogLine* line)
	{
		_mq.push(std::move(line));
	}

	void NanoLogger::set_option(LogLevel level,uint8_t field,uint8_t print)
	{
		_level = level;
		_field = field;
		_print = print;
	}

	bool NanoLogger::is_logged(LogLevel level)
	{
		return static_cast<uint8_t>(level) >= static_cast<uint8_t>(_level);
	}
} // namespace nanologger
