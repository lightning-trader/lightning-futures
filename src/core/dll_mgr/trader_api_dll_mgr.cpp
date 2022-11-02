#include "trader_api_dll_mgr.h"
#include <log_wapper.hpp>
#include <file_wapper.hpp>


bool trader_api_dll_mgr::load(const std::string& file_name)
{
	auto trader_dll_path = file_name.c_str();
	//如果没有,则再看模块目录,即dll同目录下
	if (!file_wapper::exists(trader_dll_path))
	{
		LOG_ERROR("market_dll_mgr load_dll file net exists : %s", trader_dll_path);
		return false;
	}

	_trader_handle = platform_helper::load_library(trader_dll_path);
	if (_trader_handle == nullptr)
	{
		LOG_ERROR("market_dll_mgr load_library error : %s", trader_dll_path);
		return false;
	}
	LOG_INFO("load_market_api load_library success : %s", trader_dll_path);

	create_trader_api = (create_trader_function)platform_helper::get_symbol(_trader_handle, "create_trader_api");
	if (nullptr == create_trader_api)
	{
		LOG_ERROR("load_market_api get_symbol create_trader_api error : %s", trader_dll_path);
		return false;
	}

	destory_trader_api = (destory_trader_function)platform_helper::get_symbol(_trader_handle, "destory_trader_api");
	if (nullptr == destory_trader_api)
	{
		LOG_ERROR("load_market_api get_symbol destory_trader_api error : %s", trader_dll_path);
		return false;
	}
	LOG_INFO("load_trader_api get_symbol success : %s", trader_dll_path);
	return true;
}

void trader_api_dll_mgr::unload()
{
	platform_helper::free_library(_trader_handle);
	_trader_handle = nullptr;
}