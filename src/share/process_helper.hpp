#pragma once
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

class process_helper
{
public:


#ifdef _WIN32

	static uint32_t get_pid()
	{
		return static_cast<uint32_t>(GetCurrentProcessId());
	}
#else

	static uint32_t get_pid()
	{
		return static_cast<uint32_t>(getpid());
	}
#endif
};