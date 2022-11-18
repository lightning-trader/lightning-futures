#include <trader_api.h>
#include "ctp_trader.h"

futures_trader* create_trader_api(const boost::property_tree::ptree& config)
{
	auto trader_type = config.get<std::string>("trader");
	if (trader_type == "ctp")
	{
		ctp_trader* api = new ctp_trader();
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

void destory_trader_api(futures_trader* api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
