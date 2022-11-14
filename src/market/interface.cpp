#pragma once
#include <market_api.h>
#include "ctp_market.h"

market_api* create_market_api(event_source* evt,const boost::property_tree::ptree& config)
{
	auto market_type = config.get<std::string>("market");
	if (market_type == "ctp")
	{
		ctp_market* api = new ctp_market(evt);
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

void destory_market_api(market_api* api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
