
#pragma once
#include <string>

#ifdef _MSC_VER
#include <wtypes.h>
typedef HMODULE		dll_handle;
typedef void*		process_handle;
#else
#include <dlfcn.h>
typedef void*		dll_handle;
typedef void*		process_handle;
#endif

class dll_helper
{
public:
	static dll_handle load_library(const char *filename)
	{
		//std::string dllname = get_dllname(filename);
		try
		{
#ifdef _MSC_VER
			return ::LoadLibrary(filename);
#else
			dll_handle ret = dlopen(filename, RTLD_NOW);
			if (ret == NULL)
				printf("%s\n", dlerror());
			return ret;
#endif
		}
		catch(...)
		{
			return NULL;
		}
	}

	static void free_library(dll_handle handle)
	{
		if (NULL == handle)
			return;

#ifdef _MSC_VER
		::FreeLibrary(handle);
#else
		dlclose(handle);
#endif
	}

	static process_handle get_symbol(dll_handle handle, const char* name)
	{
		if (NULL == handle)
			return NULL;

#ifdef _MSC_VER
		return ::GetProcAddress(handle, name);
#else
		return dlsym(handle, name);
#endif
	}

	static std::string get_dllname(const char* name, const char* unixPrefix = "lib")
	{
#ifdef _WIN32
		std::string ret = name;
		ret += ".dll";
		return std::move(ret);
#else
		std::size_t idx = 0;
		while (!isalpha(name[idx]))
			idx++;
		std::string ret(name, idx);
		ret.append(unixPrefix);
		ret.append(name + idx);
		ret += ".so";
		return std::move(ret);
#endif
	}
};