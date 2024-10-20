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

};