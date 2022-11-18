#include <market_api.h>
#include "ctp_market.h"

futures_market* create_market_api(const boost::property_tree::ptree& config)
{
	auto market_type = config.get<std::string>("market");
	if (market_type == "ctp")
	{
		ctp_market* api = new ctp_market();
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

void destory_market_api(futures_market* api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
