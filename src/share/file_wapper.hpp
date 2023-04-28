#pragma once
#include <boost/filesystem.hpp>


//////////////////////////////////////////////////////////////////////////
//文件辅助类
class file_wapper
{
public:
	static bool create_directory(const char* name)
	{
		if (exists(name))
			return true;

		return boost::filesystem::create_directory(boost::filesystem::path(name));
	}

	static bool create_directories(const char* name)
	{
		if (exists(name))
			return true;

		return boost::filesystem::create_directories(boost::filesystem::path(name));
	}

	static bool exists(const char* name)
	{
		return boost::filesystem::exists(boost::filesystem::path(name));
	}
};
