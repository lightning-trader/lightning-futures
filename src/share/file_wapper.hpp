#pragma once
#include <filesystem>


//////////////////////////////////////////////////////////////////////////
//文件辅助类
class file_wapper
{
public:
	static bool create_directory(const char* name)
	{
		if (exists(name))
			return true;

		return std::filesystem::create_directory(std::filesystem::path(name));
	}

	static bool create_directories(const char* name)
	{
		if (exists(name))
			return true;

		return std::filesystem::create_directories(std::filesystem::path(name));
	}

	static bool exists(const char* name)
	{
		return std::filesystem::exists(std::filesystem::path(name));
	}
};
