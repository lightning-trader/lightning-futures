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
#pragma once
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif
#include <thread>

enum class PriorityLevel
{
	LowPriority = -1,
	NormalPriority = 0,
	HighPriority,
	RealtimePriority,
};
class process_helper
{

public:


	static uint32_t get_pid()
	{
#ifdef _WIN32
		return static_cast<uint32_t>(GetCurrentProcessId());
#else
		return static_cast<uint32_t>(getpid());
#endif

	}


	// 设置进程优先级  
	static bool set_priority(PriorityLevel prio)
	{
#ifdef _WIN32
		DWORD value = ABOVE_NORMAL_PRIORITY_CLASS;
		HANDLE hProcess = GetCurrentProcess();
		switch (prio)
		{
		case PriorityLevel::RealtimePriority:
			value = REALTIME_PRIORITY_CLASS;
			break;
		case PriorityLevel::HighPriority:
			value = HIGH_PRIORITY_CLASS;
			break;
		case PriorityLevel::NormalPriority:
			value = ABOVE_NORMAL_PRIORITY_CLASS;
			break;
		case PriorityLevel::LowPriority:
			value = IDLE_PRIORITY_CLASS;
			break;

		}
		return SetPriorityClass(hProcess, value);

#elif __APPLE__ 
		pid_t pid = getpid();
		int value = 10;
		switch (prio)
		{
		case PriorityLevel::RealtimePriority:
			value = 19;
			break;
		case PriorityLevel::HighPriority:
			value = 10;
			break;
		case PriorityLevel::NormalPriority:
			value = 0;
			break;
		case PriorityLevel::LowPriority:
			value = -20;
			break;

		}
		int result = setpriority(PRIO_PROCESS, pid, value); // 
		if (result == -1)
		{
			return false;
		}
		return true;
#else
		int max_value = sched_get_priority_max(SCHED_FIFO);
		int value = 10;
		switch (prio)
		{
		case PriorityLevel::RealtimePriority:
			value = max_value;
			break;
		case PriorityLevel::HighPriority:
			value = max_value / 2;
			break;
		case PriorityLevel::NormalPriority:
			value = max_value / 3;
			break;
		case PriorityLevel::LowPriority:
			value = 1;
			break;

		}
		sched_param param;
		// param.sched_priority = prio;
		param.sched_priority = static_cast<int>(prio);
		if (sched_setscheduler(0, SCHED_FIFO, &param))
		{
			return false;
		}
#endif
		return false;
	}
	//绑核
	static bool thread_bind_core(uint32_t i)
	{
#ifdef _WIN32
		uint32_t cores = std::thread::hardware_concurrency();
		if (i >= cores)
			return false;

		HANDLE hThread = GetCurrentThread();
		DWORD_PTR mask = SetThreadAffinityMask(hThread, (DWORD_PTR)(1LLU << i));
		return (mask != 0);

#elif __APPLE__ /*ct hacked fixme: macos make error*/
		int cores = sysconf(_SC_NPROC_ONLN);
		if (cores <= 0) {
			return false;
		}
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(i, &cpuset);
		return (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0);
#else
		//int cores = get_cpu_cores();
		int cores = std::thread::hardware_concurrency();
		if (i >= cores)
			return false;
		cpu_set_t mask;
		CPU_ZERO(&mask);
		CPU_SET(i, &mask);
		return (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) == 0);
#endif
	}


	// 设置进程优先级  
	static bool set_thread_priority(PriorityLevel prio)
	{
#ifdef _WIN32
		DWORD value = THREAD_PRIORITY_TIME_CRITICAL;
		switch (prio)
		{
		case PriorityLevel::RealtimePriority:
			value = THREAD_PRIORITY_TIME_CRITICAL;
			break;
		case PriorityLevel::HighPriority:
			value = THREAD_PRIORITY_HIGHEST;
			break;
		case PriorityLevel::NormalPriority:
			value = THREAD_PRIORITY_NORMAL;
			break;
		case PriorityLevel::LowPriority:
			value = THREAD_PRIORITY_LOWEST;
			break;

		}
		HANDLE hThread = GetCurrentThread();
		return SetThreadPriority(hThread, value);

#elif __APPLE__ 
		pthread_t threadId = pthread_self();
		int value = 10;
		switch (prio)
		{
		case PriorityLevel::RealtimePriority:
			value = 20;
			break;
		case PriorityLevel::HighPriority:
			value = 15;
			break;
		case PriorityLevel::NormalPriority:
			value = 10;
			break;
		case PriorityLevel::LowPriority:
			value = 1;
			break;

		}
		struct sched_param param;
		int result = pthread_setschedparam(thread, value, &param);
		if (result != 0)
		{
			return false;
		}
		return true;
#else
		//int max_value = max_priority = sched_get_priority_max(SCHED_FIFO);
		int max_value = sched_get_priority_max(SCHED_FIFO);
		int value = 10;
		if(max_value == -1)
		{
			return false;
		}
		switch (prio)
		{
		case PriorityLevel::RealtimePriority:
			value = max_value;
			break;
		case PriorityLevel::HighPriority:
			value = max_value / 2;
			break;
		case PriorityLevel::NormalPriority:
			value = max_value / 3;
			break;
		case PriorityLevel::LowPriority:
			value = 1;
			break;

		}
		struct sched_param param;
		param.sched_priority = value; // 对于 SCHED_FIFO 或 SCHED_RR，1 是最低优先级  
		if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) != 0)
		{
			return false;
		}
#endif
		return false;
	}
};