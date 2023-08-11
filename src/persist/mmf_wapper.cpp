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
