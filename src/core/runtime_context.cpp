#include "runtime_context.h"
#include <define.h>
#include <market_api.h>
#include <trader_api.h>
#include <file_wapper.hpp>
#include <log_wapper.hpp>
#include <platform_helper.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <interface.h>

#pragma comment (lib,"trader.lib")
#pragma comment (lib,"market.lib")


runtime_context::runtime_context():_market_api(nullptr), _trader_api(nullptr)
{
}
runtime_context::~runtime_context()
{
	if (_market_api)
	{
		destory_market_api(_market_api);
		_market_api = nullptr;
	}
	if (_trader_api)
	{
		destory_trader_api(_trader_api);
		_trader_api = nullptr;
	}

}

bool runtime_context::init_from_file(const std::string& config_path)
{
	boost::property_tree::ptree	market_config;
	boost::property_tree::ptree	trader_config;
	boost::property_tree::ptree  recorder_config;

	if (!file_wapper::exists(config_path.c_str()))
	{
		LOG_ERROR("runtime_engine init_from_file config_path not exit : %s", config_path.c_str());
		return false;
	}
	try
	{
		boost::property_tree::ptree config_root;
		boost::property_tree::ini_parser::read_ini(config_path, config_root);
		market_config = config_root.get_child("market_api");
		trader_config = config_root.get_child("trader_api");
		recorder_config = config_root.get_child("recorder");
	}
	catch (...)
	{
		LOG_ERROR("runtime_engine init_from_file read_ini error : %s", config_path.c_str());
		return false;
	}
	//market
	
	_market_api = create_market_api(market_config);
	if (_market_api == nullptr)
	{
		LOG_ERROR("runtime_engine init_from_file create_market_api error : %s", config_path.c_str());
		return false;
	}
	_market_api->add_handle(std::bind(&context::handle_event, this, std::placeholders::_1, std::placeholders::_2));
	_market = _market_api;
	//trader
	
	_trader_api = create_trader_api(trader_config);
	if (_trader_api == nullptr)
	{
		LOG_ERROR("runtime_engine init_from_file create_trader_api error : %s", config_path.c_str());
		return false;
	}
	_trader_api->add_handle(std::bind(&context::handle_event, this, std::placeholders::_1, std::placeholders::_2));
	_trader = _trader_api;
	return true ;
}

void runtime_context::on_update()
{
	if(_market_api)
	{
		_market_api->update();
	}
	if(_trader_api)
	{
		_trader_api->update();
	}
}

