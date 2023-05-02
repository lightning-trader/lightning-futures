#pragma once
#include <atomic>
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

class spin_mutex
{
private:
	std::atomic<bool> flag = { false };

public:
	void lock()
	{
		for (;;)
		{
			if (!flag.exchange(true, std::memory_order_acquire))
				break;

			while (flag.load(std::memory_order_relaxed))
			{
#ifdef _MSC_VER
				_mm_pause();
#else
				__builtin_ia32_pause();
#endif
			}
		}
	}

	void unlock()
	{
		flag.store(false, std::memory_order_release);
	}


	template<typename T>
	void safe_assign(T& l, const T& r)
	{
		lock();
		l = r;
		unlock();
	}
	void safe_call(std::function<void()> callback)
	{
		lock();
		callback();
		unlock();
	}
};

class spin_lock
{
public:
	spin_lock(spin_mutex& mtx) :_mutex(mtx) { _mutex.lock(); }
	spin_lock(const spin_lock&) = delete;
	spin_lock& operator=(const spin_lock&) = delete;
	~spin_lock() { _mutex.unlock(); }

private:
	spin_mutex& _mutex;
};
