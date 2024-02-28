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
#include <mmf_wapper.hpp>
#include <fstream>
#include <map>
#include <string>
#include <filesystem>
#include <log_wapper.hpp>
#include "mio/mmap.hpp"

std::map<void*, mio::mmap_sink> _all_mmap_link;

void* maping_file(const char* path, size_t size)
{
	if (!std::filesystem::exists(path))
	{
		std::ofstream file(path);
		for(size_t i=0;i< size;i++)
		{
			file << (unsigned char)0;
		}
	}
	//自动扩容
	if(std::filesystem::file_size(path)<size)
	{
		std::ofstream file(path, std::ios::app);
		for (size_t i = std::filesystem::file_size(path); i < size; i++)
		{
			file << (unsigned char)0;
		}
	}
	std::error_code error;
	auto mms = mio::make_mmap_sink(
		path, 0, mio::map_entire_file, error);
	if (error)
	{
		LOG_FATAL("load memory map file failure error code :", error.value(), path);
		return nullptr;
	}
	void* dataptr = reinterpret_cast<void*>(mms.data());
	_all_mmap_link[dataptr] = std::move(mms);
	return dataptr;

}

void unmaping_file(void* dataptr, size_t size)
{
	auto it = _all_mmap_link.find(dataptr);
	if(it != _all_mmap_link.end())
	{
		it->second.unmap();
		//dataptr = nullptr;
		_all_mmap_link.erase(it);
	}
}
