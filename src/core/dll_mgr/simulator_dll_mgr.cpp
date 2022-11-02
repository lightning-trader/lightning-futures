#include "simulator_dll_mgr.h"
#include <log_wapper.hpp>
#include <file_wapper.hpp>


bool simulator_dll_mgr::load(const std::string& file_name)
{
	auto simulator_dll_path = file_name.c_str();
	//如果没有,则再看模块目录,即dll同目录下
	if (!file_wapper::exists(simulator_dll_path))
	{
		LOG_ERROR("market_dll_mgr load_dll file net exists : %s", simulator_dll_path);
		return false;
	}

	_simulator_handle = platform_helper::load_library(simulator_dll_path);
	if (_simulator_handle == nullptr)
	{
		LOG_ERROR("market_dll_mgr load_library error : %s", simulator_dll_path);
		return false;
	}
	LOG_INFO("load_market_api load_library success : %s", simulator_dll_path);

	create_simulator = (create_simulator_function)platform_helper::get_symbol(_simulator_handle, "create_simulator");
	if (nullptr == create_simulator)
	{
		LOG_ERROR("load_market_api get_symbol create_simulator_api error : %s", simulator_dll_path);
		return false;
	}

	destory_simulator = (destory_simulator_function)platform_helper::get_symbol(_simulator_handle, "destory_simulator");
	if (nullptr == destory_simulator)
	{
		LOG_ERROR("load_market_api get_symbol destory_simulator_api error : %s", simulator_dll_path);
		return false;
	}
	LOG_INFO("load_simulator_api get_symbol success : %s", simulator_dll_path);
	return true;
}

void simulator_dll_mgr::unload()
{
	platform_helper::free_library(_simulator_handle);
	_simulator_handle = nullptr;
}