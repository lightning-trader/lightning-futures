#include "evaluate.h"
#include "context.h"
#include <market_api.h>
#include <file_wapper.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <interface.h>

evaluate::evaluate():_market_simulator(nullptr), _trader_simulator(nullptr)
{
}
evaluate::~evaluate()
{
	if (_market_simulator)
	{
		destory_dummy_market(_market_simulator);
	}
	if (_trader_simulator)
	{
		destory_dummy_trader(_trader_simulator);
	}
}

bool evaluate::init_from_file(const std::string& config_path)
{
	boost::property_tree::ptree		market_config;
	boost::property_tree::ptree		trader_config;
	boost::property_tree::ptree  recorder_config;
	boost::property_tree::ptree  control_config;
	boost::property_tree::ptree  include_config;

	if (!file_wapper::exists(config_path.c_str()))
	{
		LOG_ERROR("evaluate_driver init_from_file config_path not exit : %s", config_path.c_str());
		return false;
	}
	try
	{
		boost::property_tree::ptree config_root;
		boost::property_tree::ini_parser::read_ini(config_path, config_root);
		market_config = config_root.get_child("dummy_market");
		trader_config = config_root.get_child("dummy_trader");
		recorder_config = config_root.get_child("recorder");
		control_config = config_root.get_child("control");
		include_config = config_root.get_child("include");
	}
	catch (...)
	{
		LOG_ERROR("evaluate_driver init_from_file read_ini error : %s", config_path.c_str());
		return false;
	}
	//simulator

	
	_trader_simulator = create_dummy_trader(trader_config);
	if (_trader_simulator == nullptr)
	{
		LOG_ERROR("evaluate_driver init_from_file create_dummy_trader error : %s", config_path.c_str());
		return false;
	}

	_market_simulator = create_dummy_market(market_config);
	if (_market_simulator == nullptr)
	{
		LOG_ERROR("evaluate_driver init_from_file create_dummy_market error : %s", config_path.c_str());
		return false;
	}
	this->init(control_config, include_config, recorder_config, true);
	return true;
}


void evaluate::playback_history(uint32_t tradeing_day)
{
	if (_trader_simulator)
	{
		_trader_simulator->crossday(tradeing_day);
	}
	if(_market_simulator)
	{	
		_market_simulator->play(tradeing_day,[this](const tick_info& tick)->void{
			_trader_simulator->push_tick(tick);
		});
	}
}

trader_api& evaluate::get_trader()
{
	return *_trader_simulator;
}

market_api& evaluate::get_market()
{
	return *_market_simulator;
}

void evaluate::on_update()
{
	if(_market_simulator)
	{
		_market_simulator->update();
	}
	if (is_in_trading()&& _trader_simulator)
	{
		while (!_trader_simulator->is_empty())
		{
			_trader_simulator->update();
		}
	}
}

bool evaluate::is_terminaled()
{
	if(_trader_simulator)
	{
		return _trader_simulator->is_empty();
	}
	return false;
}

void evaluate::add_market_handle(std::function<void(market_event_type, const std::vector<std::any>&)> handle)
{
	if (_market_simulator)
	{
		_market_simulator->add_handle(handle);
	}
}

void evaluate::add_trader_handle(std::function<void(trader_event_type, const std::vector<std::any>&)> handle)
{
	if (_trader_simulator)
	{
		_trader_simulator->add_handle(handle);
	}
}