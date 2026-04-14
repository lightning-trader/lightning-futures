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

#ifndef NANO_LOG_HEADER_GUARD
#define NANO_LOG_HEADER_GUARD

#include <atomic>
#include <thread>
#include <memory>
#include <mutex>
#include <mpsc_queue.hpp>
#include "log_define.hpp"

namespace nanolog
{

	class LogWriter
	{
	public:
		virtual void write(const NanoLogLine& logline, uint8_t field) = 0;
		virtual void flush() = 0;
	};

	class NanoLogger

	{

	public:

		NanoLogger() = default;

		NanoLogger(std::atomic<NanoLogger*>& holder,  std::string const& log_directory, uint32_t file_size_mb);

		~NanoLogger();
		
		void set_option(LogLevel level, uint8_t field, uint8_t print);

		bool is_logged(LogLevel level);

		uint8_t field_mask() const;
		
		void pop();
		
		NanoLogLine* alloc();
		
		void recycle(NanoLogLine* line);
		
		void dump(NanoLogLine* line);

		void flush(uint32_t timeout_ms = 1000U);
		
	private:

		struct free_list_head
		{
			NanoLogLine* line;
			uint64_t version;
		};

		static constexpr size_t LOG_POOL_SIZE = 4096U;

		void write_to_outputs(const NanoLogLine& logline, bool flush_immediately);

		NanoLogLine* pop_free_line();

		void push_free_line(NanoLogLine* line);


		std::unique_ptr<LogWriter> _file_writer;

		std::unique_ptr<LogWriter> _console_writer;

		std::unique_ptr<std::thread> _thread;

		mpsc::mpsc_queue<NanoLogLine> _mq;

		std::atomic<uint64_t> _pending_count = 0U;

		std::atomic<bool> _is_runing = false;

		std::thread::id _worker_thread_id{};
		
		LogLevel _level = LogLevel::LLV_TRACE;
		uint8_t _field = 0U;
		uint8_t _print = 0U;
		std::mutex _writer_mutex;
		std::unique_ptr<NanoLogLine[]> _pool_storage;
		std::atomic<free_list_head> _free_list = free_list_head{ nullptr, 0U };
	};



} // namespace nanolog

#endif /* NANO_LOG_HEADER_GUARD */

