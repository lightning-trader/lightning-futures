/*

Distributed under the MIT License (MIT)

	Copyright (c) 2016 Karthik Iyengar

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

#ifndef NANO_LOG_HEADER_GUARD
#define NANO_LOG_HEADER_GUARD

#include <thread>
#include <log_wapper.hpp>

namespace nanolog
{
	enum class LogField : uint8_t
	{
		TIME_SPAMP = 0B00000001,
		THREAD_ID = 0B00000010,
		LOG_LEVEL = 0B00000100,
		SOURCE_FILE = 0B00001000,
		FUNCTION = 0B00010000,
	};


	enum class LogPrint : uint8_t
	{
		LOG_FILE = 0B00000001,
		CONSOLE = 0B00000010,
	};

	class LogWriter
	{
	public:
		virtual void write(const NanoLogLine& logline, uint8_t field) = 0;
	};

	class NanoLogger
	{

	public:

		NanoLogger() = default;

		NanoLogger(std::string const& log_directory, std::string const& log_file_name, uint32_t file_size_mb,size_t pool_size);

		~NanoLogger();
		
		void set_option(LogLevel level, uint8_t field, uint8_t print);

		bool is_logged(LogLevel level);

		void pop();
		
		NanoLogLine* alloc();
		
		void recycle(NanoLogLine* line);
		
		void dump(NanoLogLine* line);
		
	private:


		std::unique_ptr<LogWriter> _file_writer;

		std::unique_ptr<LogWriter> _console_writer;

		std::unique_ptr<std::thread> _thread;

		LoglinePool _lp;
		LoglineQueue _mq;

		std::atomic<bool> _is_runing = false;
		
		LogLevel _level = LogLevel::LLV_TRACE;
		uint8_t _field = 0U;
		uint8_t _print = 0U;
	};



} // namespace nanolog

#endif /* NANO_LOG_HEADER_GUARD */

