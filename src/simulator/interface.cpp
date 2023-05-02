#include "market_simulator.h"
#include "trader_simulator.h"
#include <interface.h>

dummy_market* create_dummy_market(const boost::property_tree::ptree& config)
{
	market_simulator* api = new market_simulator();
	if (api->init(config))
	{
		return api;
	}
	else
	{
		delete api;
	}
	return nullptr;
}

void destory_dummy_market(dummy_market*& api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
dummy_trader* create_dummy_trader(const boost::property_tree::ptree& config, trader_data& ret_data)
{
	trader_simulator* api = new trader_simulator();
	if (api->init(config, ret_data))
	{
		return api;
	}
	else
	{
		delete api;
	}
	return nullptr;
}

void destory_dummy_trader(dummy_trader*& api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
