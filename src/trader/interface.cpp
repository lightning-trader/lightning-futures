#pragma once
#include <trader_api.h>
#include "ctp_trader.h"

trader_api* create_trader_api(event_source* evt, const boost::property_tree::ptree& config)
{
	auto trader_type = config.get<std::string>("trader");
	if (trader_type == "ctp")
	{
		ctp_trader* api = new ctp_trader(evt);
		if (api->init(config))
		{
			return api;
		}
		else
		{
			delete api;
		}
	}
	return nullptr;
}

void destory_trader_api(trader_api* api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
