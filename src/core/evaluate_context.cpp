#include "evaluate_context.h"
#include <simulator.h>
#include <log_wapper.hpp>
#include <file_wapper.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <interface.h>

#pragma comment (lib,"simulator.lib")

evaluate_context::evaluate_context():_simulator(nullptr)
{
}
evaluate_context::~evaluate_context()
{
	if (_simulator)
	{
		destory_simulator(_simulator);
		_simulator = nullptr;
	}

}

bool evaluate_context::init_from_file(const std::string& config_path)
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

	_simulator = create_simulator(simulator_config);
	if (_simulator == nullptr)
	{
		LOG_ERROR("evaluate_driver init_from_file create_simulator error : %s", config_path.c_str());
		return false;
	}
	_simulator->add_handle(std::bind(&context::handle_event, this, std::placeholders::_1, std::placeholders::_2));
	_trader = _simulator;
	_market = _simulator;
	return true;
	
}

void evaluate_context::on_update()
{
	if(_simulator)
	{
		_simulator->update();
	}
}


double evaluate_context::get_money()
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

void evaluate_context::play(uint32_t tradeing_day)
{
	if(_simulator)
	{
		_simulator->set_trading_day(tradeing_day);
		_simulator->play();
	}
}