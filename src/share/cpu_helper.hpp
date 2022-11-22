#pragma once
#include <thread>
#ifdef _WIN32
#include <wtypes.h>
#include <WinBase.h>
#else
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#endif

class cpu_helper
{
public:
	static uint32_t get_cpu_cores()
	{
		static uint32_t cores = std::thread::hardware_concurrency();
		return cores;
	}

#ifdef _WIN32

	static bool bind_core(uint32_t i)
	{
		uint32_t cores = get_cpu_cores();
		if (i >= cores)
			return false;

		HANDLE hThread = GetCurrentThread();
		DWORD_PTR mask = SetThreadAffinityMask(hThread, (DWORD_PTR)(1LLU << i));
		return (mask != 0);
	}
#else

	static bool bind_core(uint32_t i)
	{
		int cores = get_cpu_cores();
		if (i >= cores)
			return false;

		cpu_set_t mask;
		CPU_ZERO(&mask);
		CPU_SET(i, &mask);
		return (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) >= 0);
	}
#endif
};