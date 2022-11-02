#include "evaluate_driver.h"
#include <define.h>
#include <simulator.h>
#include <log_wapper.hpp>
#include <file_wapper.hpp>
#include <boost/property_tree/ini_parser.hpp>

evaluate_driver::evaluate_driver():_simulator(nullptr), _recorder(nullptr)
{
}
evaluate_driver::~evaluate_driver()
{
	if (_simulator)
	{
		_simulator_dll.destory_simulator(_simulator);
		_simulator = nullptr;
	}
	/*
	if (_recorder)
	{
		_recorder_dll.destory_recorder(_recorder);
		_recorder = nullptr;
	}
	*/
	_simulator_dll.unload();
	//_recorder_dll.unload();
}

bool evaluate_driver::init_from_file(const std::string& config_path)
{
	boost::property_tree::ptree	simulator_config;
	boost::property_tree::ptree  recorder_config;

	if (!file_wapper::exists(config_path.c_str()))
	{
		LOG_ERROR("evaluate_driver init_from_file config_path not exit : %s", config_path.c_str());
		return false;
	}
	try
	{
		boost::property_tree::ptree config_root;
		boost::property_tree::ini_parser::read_ini(config_path, config_root);
		simulator_config = config_root.get_child("simulator");
		recorder_config = config_root.get_child("recorder");
	}
	catch (...)
	{
		LOG_ERROR("evaluate_driver init_from_file read_ini error : %s", config_path.c_str());
		return false;
	}
	//simulator
	if (!_simulator_dll.load("./simulator.dll"))
	{
		LOG_ERROR("evaluate_driver init_from_file _market_dll load error : %s", config_path.c_str());
		return false;
	}
	_simulator = _simulator_dll.create_simulator(simulator_config);
	if (_simulator == nullptr)
	{
		LOG_ERROR("evaluate_driver init_from_file create_simulator error : %s", config_path.c_str());
		return false;
	}
	/*
	//recorder
	if (!_recorder_dll.load("./recorder.dll"))
	{
		LOG_ERROR("runtime_engine init_from_file _recorder_dll load error : %s", config_path.c_str());
		return false;
	}
	_recorder = _recorder_dll.create_recorder(recorder_config);
	if (_recorder == nullptr)
	{
		LOG_ERROR("runtime_engine init create_recorder error : %s", config_path.c_str());
		return false;
	}
	*/
	return true;
	
}

void evaluate_driver::update()
{
	if(_simulator)
	{
		_simulator->update();
	}
}

void evaluate_driver::add_handle(std::function<void(event_type, const std::vector<std::any>&)> handle)
{
	if (_simulator)
	{
		_simulator->add_handle(handle);
	}
}

void evaluate_driver::set_trading_day(uint32_t tradeing_day)
{
	if (_simulator)
	{
		_simulator->set_trading_day(tradeing_day);
	}
}

double evaluate_driver::get_money()
{
	if (_simulator)
	{
		auto acc = _simulator->get_account();
		if(acc)
		{
			return acc->money ;
		}
	}
	return 0;
}

void evaluate_driver::play()
{
	if(_simulator)
	{
		_simulator->play();
	}
}