#include "market_api_dll_mgr.h"
#include <log_wapper.hpp>
#include <file_wapper.hpp>


bool market_api_dll_mgr::load(const std::string& file_name)
{
	auto market_dll_path = file_name.c_str();
	//如果没有,则再看模块目录,即dll同目录下
	if (!file_wapper::exists(market_dll_path))
	{
		LOG_ERROR("market_dll_mgr load_dll file net exists : %s", market_dll_path);
		return false;
	}

	_market_handle = platform_helper::load_library(market_dll_path);
	if (_market_handle == nullptr)
	{
		LOG_ERROR("market_dll_mgr load_library error : %s", market_dll_path);
		return false;
	}
	LOG_INFO("load_market_api load_library success : %s", market_dll_path);

	create_market_api = (create_market_function)platform_helper::get_symbol(_market_handle, "create_market_api");
	if (nullptr == create_market_api)
	{
		LOG_ERROR("load_market_api get_symbol create_market_api error : %s", market_dll_path);
		return false;
	}

	destory_market_api = (destory_market_function)platform_helper::get_symbol(_market_handle, "destory_market_api");
	if (nullptr == destory_market_api)
	{
		LOG_ERROR("load_market_api get_symbol destory_market_api error : %s", market_dll_path);
		return false;
	}
	LOG_INFO("load_market_api get_symbol success : %s", market_dll_path);
	return true;
}

void market_api_dll_mgr::unload()
{
	platform_helper::free_library(_market_handle);
	_market_handle = nullptr;
}