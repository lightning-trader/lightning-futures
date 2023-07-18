#include <market_api.h>
#include "ctp_market.h"

actual_market* create_actual_market(const params& config)
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

void destory_actual_market(actual_market*& api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
