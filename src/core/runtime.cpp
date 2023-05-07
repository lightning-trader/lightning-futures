#include "runtime.h"
#include <define.h>
#include <market_api.h>
#include <trader_api.h>
#include <file_wapper.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <interface.h>


runtime::runtime():_market(nullptr), _trader(nullptr)
{
}
runtime::~runtime()
{
	if (_market)
	{
		destory_actual_market(_market);
	}
	if (_trader)
	{
		destory_actual_trader(_trader);
	}

}

bool runtime::init_from_file(const std::string& config_path)
{
	boost::property_tree::ptree	market_config;
	boost::property_tree::ptree	trader_config;
	boost::property_tree::ptree  recorder_config;
	boost::property_tree::ptree  control_config;
	boost::property_tree::ptree  include_config;

	if (!file_wapper::exists(config_path.c_str()))
	{
		LOG_ERROR("runtime_engine init_from_file config_path not exit : %s", config_path.c_str());
		return false;
	}
	try
	{
		boost::property_tree::ptree config_root;
		boost::property_tree::ini_parser::read_ini(config_path, config_root);
		market_config = config_root.get_child("actual_market");
		trader_config = config_root.get_child("actual_trader");
		recorder_config = config_root.get_child("recorder");
		control_config = config_root.get_child("control");
		include_config = config_root.get_child("include");
	}
	catch (...)
	{
		LOG_ERROR("runtime_engine init_from_file read_ini error : %s", config_path.c_str());
		return false;
	}
	//trader
	_trader = create_actual_trader(trader_config);
	if (_trader == nullptr)
	{
		LOG_ERROR("runtime_engine init_from_file create_trader_api error : %s", config_path.c_str());
		return false;
	}

	//market
	_market = create_actual_market(market_config);
	if (_market == nullptr)
	{
		LOG_ERROR("runtime_engine init_from_file create_market_api error : %s", config_path.c_str());
		return false;
	}
	this->init(control_config, include_config, recorder_config);
	return true;
}

trader_api& runtime::get_trader()
{
	return *_trader;
}

market_api& runtime::get_market()
{
	return *_market;
}

void runtime::on_update()
{
	if (_market)
	{
		_market->update();
	}
	if (is_in_trading() && _trader)
	{
		while (!_trader->is_empty())
		{
			_trader->update();
		}
	}
}

bool runtime::is_terminaled()
{
	if (_trader)
	{
		return _trader->is_empty();
	}
	return false;
}

void runtime::add_market_handle(std::function<void(market_event_type, const std::vector<std::any>&)> handle)
{
	if(_market)
	{
		_market->add_handle(handle);
	}
}

void runtime::add_trader_handle(std::function<void(trader_event_type, const std::vector<std::any>&)> handle)
{
	if (_trader)
	{
		_trader->add_handle(handle);
	}
}
