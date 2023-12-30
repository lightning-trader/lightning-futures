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

#include "nanolog.hpp"
#include <cstring>
#include <chrono>
#include <ctime>
#include <tuple>
#include <atomic>
#include <queue>
#include <fstream>
#include <iostream>
#include <log_wapper.hpp>

namespace
{

	/* Returns microseconds since epoch */
	uint64_t timestamp_now()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

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

	std::thread::id this_thread_id()
	{
		static thread_local const std::thread::id id = std::this_thread::get_id();
		return id;
	}

} // anonymous namespace

namespace nanolog
{
	char const* to_string(LogLevel loglevel)
	{

		switch (loglevel)
		{
		case LogLevel::LOG_TRACE:
			return "TRACE";
		case LogLevel::LOG_DEBUG:
			return "DEBUG";
		case LogLevel::LOG_INFO:
			return "INFO";
		case LogLevel::LOG_WARNING:
			return "WARNING";
		case LogLevel::LOG_ERROR:
			return "ERROR";
		case LogLevel::LOG_FATAL:
			return "FATAL";
		}
		return "XXXX";
	}
	NanoLogLine::NanoLogLine() : m_log_level(LogLevel::LOG_TRACE)
		, m_source_file("")
		, m_function("")
		, m_source_line(0)
	{
		memset(m_buffer, 0, LOG_BUFFER_SIZE);
		m_timestamp = timestamp_now();
		m_thread_id = this_thread_id();
	}
	
	NanoLogLine::NanoLogLine(LogLevel level, char const* file, char const* function, uint32_t line,const unsigned char * msg_data)
		: m_log_level(level)
		, m_source_file(file)
		,m_function(function)
		,m_source_line(line)
	{
		memset(m_buffer, 0, LOG_BUFFER_SIZE);
		memcpy(m_buffer, msg_data, LOG_BUFFER_SIZE);
		m_timestamp = timestamp_now();
		m_thread_id = this_thread_id();
	}

	NanoLogLine::~NanoLogLine()
	{
		//delete[] m_buffer;
	}

	void NanoLogLine::stringify(std::ostream& os,uint8_t field)
	{
		if (field & static_cast<uint8_t>(LogField::TIME_SPAMP))
		{
			format_timestamp(os, m_timestamp);
		}
		if (field & static_cast<uint8_t>(LogField::THREAD_ID))
		{
			os << '[' << m_thread_id << ']';
		}
		if (field & static_cast<uint8_t>(LogField::LOG_LEVEL))
		{
			os << '[' << to_string(m_log_level) << ']';
		}
		if (field & static_cast<uint8_t>(LogField::SOURCE_FILE))
		{
			os << '[' << m_source_file << ':' << m_source_line << "] ";
		}
		if (field & static_cast<uint8_t>(LogField::FUNCTION))
		{
			os << '[' << m_function << ':' << m_source_line << "] ";
		}
		stream_extractor sd(m_buffer, LOG_BUFFER_SIZE);
		sd.out(os);
		os << std::endl;
		os.flush();
	}

	

	struct BufferBase
	{
		virtual ~BufferBase() = default;
		virtual void push(NanoLogLine&& logline) = 0;
		virtual bool try_pop(NanoLogLine& logline) = 0;
	};

	struct SpinLock
	{
		SpinLock(std::atomic_flag& flag) : m_flag(flag)
		{
			while (m_flag.test_and_set(std::memory_order_acquire));
		}

		~SpinLock()
		{
			m_flag.clear(std::memory_order_release);
		}

	private:
		std::atomic_flag& m_flag;
	};

	/* Multi Producer Single Consumer Ring Buffer */
	class RingBuffer : public BufferBase
	{
	public:
		struct alignas(64) Item
		{
			Item()
				: flag{ ATOMIC_FLAG_INIT }
				, written(0)
				, padding{0}
			{
			}

			std::atomic_flag flag;
			char written;
			char padding[LOG_BUFFER_SIZE];
			NanoLogLine logline;
		};

		RingBuffer(size_t const size)
			: m_size(size)
			, m_ring(static_cast<Item*>(std::malloc(size * sizeof(Item))))
			, m_write_index(0)
			, m_read_index(0)
			, pad{0}
		{
			for (size_t i = 0; i < m_size; ++i)
			{
				new (&m_ring[i]) Item();
			}
		}

		~RingBuffer()
		{
			for (size_t i = 0; i < m_size; ++i)
			{
				m_ring[i].~Item();
			}
			std::free(m_ring);
		}

		void push(NanoLogLine&& logline) override
		{
			unsigned int write_index = m_write_index.fetch_add(1, std::memory_order_relaxed) % m_size;
			Item& item = m_ring[write_index];
			SpinLock spinlock(item.flag);
			item.logline = std::move(logline);
			item.written = 1;
		}

		bool try_pop(NanoLogLine& logline) override
		{
			Item& item = m_ring[m_read_index % m_size];
			SpinLock spinlock(item.flag);
			if (item.written == 1)
			{
				logline = std::move(item.logline);
				item.written = 0;
				++m_read_index;
				return true;
			}
			return false;
		}

		RingBuffer(RingBuffer const&) = delete;
		RingBuffer& operator=(RingBuffer const&) = delete;

	private:
		size_t const m_size;
		Item* m_ring;
		std::atomic < unsigned int > m_write_index;
		char pad[64];
		unsigned int m_read_index;
	};


	class Buffer
	{
	public:
		struct Item
		{
			Item(NanoLogLine&& nanologline) : logline(std::move(nanologline)) {}
			NanoLogLine logline;
		};

		static constexpr const size_t size = 32768; // 8MB. Helps reduce memory fragmentation

		Buffer() : m_buffer(static_cast<Item*>(std::malloc(size * sizeof(Item))))
		{
			for (size_t i = 0; i <= size; ++i)
			{
				m_write_state[i].store(0, std::memory_order_relaxed);
			}
		}

		~Buffer()
		{
			unsigned int write_count = m_write_state[size].load();
			for (size_t i = 0; i < write_count; ++i)
			{
				m_buffer[i].~Item();
			}
			std::free(m_buffer);
		}

		// Returns true if we need to switch to next buffer
		bool push(NanoLogLine&& logline, unsigned int const write_index)
		{
			new (&m_buffer[write_index]) Item(std::move(logline));
			m_write_state[write_index].store(1, std::memory_order_release);
			return m_write_state[size].fetch_add(1, std::memory_order_acquire) + 1 == size;
		}

		bool try_pop(NanoLogLine& logline, unsigned int const read_index)
		{
			if (m_write_state[read_index].load(std::memory_order_acquire))
			{
				Item& item = m_buffer[read_index];
				logline = std::move(item.logline);
				return true;
			}
			return false;
		}

		Buffer(Buffer const&) = delete;
		Buffer& operator=(Buffer const&) = delete;

	private:
		Item* m_buffer;
		std::atomic < unsigned int > m_write_state[size + 1];
	};

	class QueueBuffer : public BufferBase
	{
	public:
		QueueBuffer(QueueBuffer const&) = delete;
		QueueBuffer& operator=(QueueBuffer const&) = delete;

		QueueBuffer() : m_current_read_buffer{ nullptr }
			, m_write_index(0)
			, m_flag{ ATOMIC_FLAG_INIT }
			, m_read_index(0)
		{
			setup_next_write_buffer();
		}

		void push(NanoLogLine&& logline) override
		{
			unsigned int write_index = m_write_index.fetch_add(1, std::memory_order_relaxed);
			if (write_index < Buffer::size)
			{
				if (m_current_write_buffer.load(std::memory_order_acquire)->push(std::move(logline), write_index))
				{
					setup_next_write_buffer();
				}
			}
			else
			{
				while (m_write_index.load(std::memory_order_acquire) >= Buffer::size);
				push(std::move(logline));
			}
		}

		bool try_pop(NanoLogLine& logline) override
		{
			if (m_current_read_buffer == nullptr)
				m_current_read_buffer = get_next_read_buffer();

			Buffer* read_buffer = m_current_read_buffer;

			if (read_buffer == nullptr)
				return false;

			if (bool success = read_buffer->try_pop(logline, m_read_index))
			{
				m_read_index++;
				if (m_read_index == Buffer::size)
				{
					m_read_index = 0;
					m_current_read_buffer = nullptr;
					SpinLock spinlock(m_flag);
					m_buffers.pop();
				}
				return true;
			}

			return false;
		}

	private:
		void setup_next_write_buffer()
		{
			std::unique_ptr < Buffer > next_write_buffer(new Buffer());
			m_current_write_buffer.store(next_write_buffer.get(), std::memory_order_release);
			SpinLock spinlock(m_flag);
			m_buffers.push(std::move(next_write_buffer));
			m_write_index.store(0, std::memory_order_relaxed);
		}

		Buffer* get_next_read_buffer()
		{
			SpinLock spinlock(m_flag);
			return m_buffers.empty() ? nullptr : m_buffers.front().get();
		}

	private:
		std::queue < std::unique_ptr < Buffer > > m_buffers;
		std::atomic < Buffer* > m_current_write_buffer;
		Buffer* m_current_read_buffer;
		std::atomic < unsigned int > m_write_index;
		std::atomic_flag m_flag;
		unsigned int m_read_index;
	};

	class FileWriter
	{
	public:
		FileWriter(std::string const& log_directory, std::string const& log_file_name, uint32_t log_file_roll_size_mb)
			: m_log_file_roll_size_bytes(log_file_roll_size_mb * 1024 * 1024)
			, m_name(log_directory + log_file_name)
		{
			roll_file();
		}

		void write(NanoLogLine& logline,uint8_t field)
		{
			auto pos = m_os->tellp();
			logline.stringify(*m_os, field);
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

	class NanoLogger
	{
	public:
		NanoLogger(NonGuaranteedLogger ngl, std::string const& log_directory, std::string const& log_file_name, uint32_t log_file_roll_size_mb)
			: m_state(State::INIT)
			, m_buffer_base(new RingBuffer(std::max(1u, ngl.ring_buffer_size_mb) * 1024 * 4))
			, m_file_writer(log_directory, log_file_name, std::max(1u, log_file_roll_size_mb))
			, m_thread(&NanoLogger::pop, this)
			, m_log_field(0)
			, m_log_print(0)
		{
			m_state.store(State::READY, std::memory_order_release);
		}

		NanoLogger(GuaranteedLogger gl, std::string const& log_directory, std::string const& log_file_name, uint32_t log_file_roll_size_mb)
			: m_state(State::INIT)
			, m_buffer_base(new QueueBuffer())
			, m_file_writer(log_directory, log_file_name, std::max(1u, log_file_roll_size_mb))
			, m_thread(&NanoLogger::pop, this)
			, m_log_field(0)
			, m_log_print(0)
		{
			m_state.store(State::READY, std::memory_order_release);
		}

		~NanoLogger()
		{
			m_state.store(State::SHUTDOWN);
			m_thread.join();
			std::cout.flush();
		}

		void add(NanoLogLine&& logline)
		{
			m_buffer_base->push(std::move(logline));
		}

		void pop()
		{
			// Wait for constructor to complete and pull all stores done there to this thread / core.
			while (m_state.load(std::memory_order_acquire) == State::INIT)
				std::this_thread::sleep_for(std::chrono::microseconds(30));

			NanoLogLine logline;

			while (m_state.load() == State::READY)
			{
				if (m_buffer_base->try_pop(logline)) 
				{
					if (m_log_print & static_cast<uint8_t>(LogPrint::LOG_FILE))
					{
						m_file_writer.write(logline, m_log_field);
					}
					if (m_log_print & static_cast<uint8_t>(LogPrint::CONSOLE))
					{
						logline.stringify(std::cout, m_log_field);
					}
				}
				else 
				{
					std::this_thread::yield();
				}
			}

			// Pop and log all remaining entries
			while (m_buffer_base->try_pop(logline))
			{
				if (m_log_print & static_cast<uint8_t>(LogPrint::LOG_FILE))
				{
					m_file_writer.write(logline, m_log_field);
				}
				if (m_log_print & static_cast<uint8_t>(LogPrint::CONSOLE))
				{
					logline.stringify(std::cout,m_log_field);
				}
			}
		}


	private:
		enum class State
		{
			INIT,
			READY,
			SHUTDOWN
		};

		std::atomic < State > m_state;
		std::unique_ptr < BufferBase > m_buffer_base;
		FileWriter m_file_writer;
		std::thread m_thread;

	public:
		uint8_t m_log_field;
		uint8_t m_log_print;
	};

	std::unique_ptr < NanoLogger > nanologger;
	std::atomic < NanoLogger* > atomic_nanologger;
	
	void add_logline(NanoLogLine&& logline)
	{
		atomic_nanologger.load(std::memory_order_acquire)->add(std::move(logline));
	}

	void initialize(NonGuaranteedLogger ngl, std::string const& log_directory, std::string const& log_file_name, uint32_t log_file_roll_size_mb)
	{
		nanologger.reset(new NanoLogger(ngl, log_directory, log_file_name, log_file_roll_size_mb));
		atomic_nanologger.store(nanologger.get(), std::memory_order_seq_cst);
	}

	void initialize(GuaranteedLogger gl, std::string const& log_directory, std::string const& log_file_name, uint32_t log_file_roll_size_mb)
	{
		nanologger.reset(new NanoLogger(gl, log_directory, log_file_name, log_file_roll_size_mb));
		atomic_nanologger.store(nanologger.get(), std::memory_order_seq_cst);
	}

	std::atomic < unsigned int > loglevel = { 0 };

	void set_log_option(LogLevel level,uint8_t field,uint8_t print)
	{
		loglevel.store(static_cast<unsigned int>(level), std::memory_order_release);
		nanologger->m_log_field = field;
		nanologger->m_log_print = print;
	}

	bool is_logged(LogLevel level)
	{
		return static_cast<unsigned int>(level) >= loglevel.load(std::memory_order_relaxed);
	}
} // namespace nanologger
