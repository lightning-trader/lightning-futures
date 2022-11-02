#include "recorder_dll_mgr.h"
#include <log_wapper.hpp>
#include <file_wapper.hpp>


bool recorder_dll_mgr::load(const std::string& file_name)
{
	auto recorder_dll_path = file_name.c_str();
	//如果没有,则再看模块目录,即dll同目录下
	if (!file_wapper::exists(recorder_dll_path))
	{
		LOG_ERROR("market_dll_mgr load_dll file net exists : %s", recorder_dll_path);
		return false;
	}

	_recorder_handle = platform_helper::load_library(recorder_dll_path);
	if (_recorder_handle == nullptr)
	{
		LOG_ERROR("market_dll_mgr load_library error : %s", recorder_dll_path);
		return false;
	}
	LOG_INFO("load_market_api load_library success : %s", recorder_dll_path);

	create_recorder = (create_recorder_function)platform_helper::get_symbol(_recorder_handle, "create_recorder");
	if (nullptr == create_recorder)
	{
		LOG_ERROR("load_market_api get_symbol create_recorder_api error : %s", recorder_dll_path);
		return false;
	}

	destory_recorder = (destory_recorder_function)platform_helper::get_symbol(_recorder_handle, "destory_recorder");
	if (nullptr == destory_recorder)
	{
		LOG_ERROR("load_market_api get_symbol destory_recorder_api error : %s", recorder_dll_path);
		return false;
	}
	LOG_INFO("load_recorder_api get_symbol success : %s", recorder_dll_path);
	return true;
}

void recorder_dll_mgr::unload()
{
	platform_helper::free_library(_recorder_handle);
	_recorder_handle = nullptr;
}