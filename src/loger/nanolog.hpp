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

#include <cstdint>
#include <memory>
#include <string>
#include <iosfwd>
#include <thread>
#include <type_traits>

namespace nanolog
{
	enum class LogLevel : uint8_t 
	{
		LOG_TRACE = 0U,
		LOG_DEBUG = 1U,
		LOG_INFO = 2U,
		LOG_WARNING = 3U,
		LOG_ERROR = 4U,
		LOG_FATAL = 5U,
	};
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

	class NanoLogLine
	{
	public:

		NanoLogLine();

		NanoLogLine(LogLevel level, char const* file, char const* function, uint32_t line, unsigned char* msg_data);

		~NanoLogLine();

		NanoLogLine(NanoLogLine&&) = default;

		NanoLogLine& operator=(NanoLogLine&&) = default;

		void stringify(std::ostream& os, uint8_t field);

		unsigned char* m_buffer;

	private:
		uint64_t m_timestamp;
		std::thread::id m_thread_id;
		LogLevel m_log_level;
		const char* m_source_file;
		const char* m_function;
		uint32_t	m_source_line;
		
	};

	void add_logline(NanoLogLine&& logline);

	void set_log_option(LogLevel level, uint8_t field, uint8_t print);

	bool is_logged(LogLevel level);

	unsigned char* alloc_buffer();

	void free_buffer(unsigned char*& dataptr);

	/*
	 * Non guaranteed logging. Uses a ring buffer to hold log lines.
	 * When the ring gets full, the previous log line in the slot will be dropped.
	 * Does not block producer even if the ring buffer is full.
	 * ring_buffer_size_mb - LogLines are pushed into a mpsc ring buffer whose size
	 * is determined by this parameter. Since each LogLine is 256 bytes,
	 * ring_buffer_size = ring_buffer_size_mb * 1024 * 1024 / 256
	 */
	struct NonGuaranteedLogger
	{
		NonGuaranteedLogger(uint32_t ring_buffer_size_mb_) : ring_buffer_size_mb(ring_buffer_size_mb_) {}
		uint32_t ring_buffer_size_mb;
	};

	/*
	 * Provides a guarantee log lines will not be dropped.
	 */
	struct GuaranteedLogger
	{
	};

	/*
	 * Ensure initialize() is called prior to any log statements.
	 * log_directory - where to create the logs. For example - "/tmp/"
	 * log_file_name - root of the file name. For example - "nanolog"
	 * This will create log files of the form -
	 * /tmp/nanolog.1.txt
	 * /tmp/nanolog.2.txt
	 * etc.
	 * log_file_roll_size_mb - mega bytes after which we roll to next log file.
	 */
	void initialize(GuaranteedLogger gl, std::string const& log_directory, std::string const& log_file_name, uint32_t log_file_roll_size_mb);
	void initialize(NonGuaranteedLogger ngl, std::string const& log_directory, std::string const& log_file_name, uint32_t log_file_roll_size_mb);

} // namespace nanolog

#endif /* NANO_LOG_HEADER_GUARD */

