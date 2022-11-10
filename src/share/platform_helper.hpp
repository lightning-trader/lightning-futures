#pragma once
#include <thread>
#include <string>
#ifdef _MSC_VER
#include <wtypes.h>
#include <windows.h>
typedef HMODULE		DllHandle;
typedef void* ProcHandle;
#else
#include <dlfcn.h>
typedef void* DllHandle;
typedef void* ProcHandle;
#endif

class platform_helper
{
public:
	static uint32_t get_cpu_cores()
	{
		static uint32_t cores = std::thread::hardware_concurrency();
		return cores;
	}

#ifdef _WIN32
#include <thread>
	static bool bind_core(uint64_t i)
	{
		uint32_t cores = get_cpu_cores();
		if (i >= cores)
			return false;

		HANDLE hThread = GetCurrentThread();
		DWORD_PTR mask = SetThreadAffinityMask(hThread, (DWORD_PTR)(1LL << i));
		return (mask != 0);
	}
#else
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
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

	static DllHandle load_library(const char* filename)
	{
		try
		{
#ifdef _MSC_VER
			return ::LoadLibrary(filename);
#else
			DllHandle ret = dlopen(filename, RTLD_NOW);
			if (ret == NULL)
				printf("%s\n", dlerror());
			return ret;
#endif
		}
		catch (...)
		{
			return NULL;
		}
	}

	static void free_library(DllHandle handle)
	{
		if (NULL == handle)
			return;

#ifdef _MSC_VER
		::FreeLibrary(handle);
#else
		dlclose(handle);
#endif
	}

	static ProcHandle get_symbol(DllHandle handle, const char* name)
	{
		if (NULL == handle)
			return NULL;

#ifdef _MSC_VER
		return ::GetProcAddress(handle, name);
#else
		return dlsym(handle, name);
#endif
	}

	static std::string wrap_module(const char* name, const char* unixPrefix = "lib")
	{

#ifdef _WIN32
		std::string ret = name;
		ret += ".dll";
		return std::move(ret);
#else
		std::string ret(unixPrefix);
		ret += name;
		ret += ".so";
		return std::move(ret);
#endif
	}


};

