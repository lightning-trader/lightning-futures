#include <trader_api.h>
#include "ctp_trader.h"
#include <interface.h>

actual_trader* create_actual_trader(const params& config)
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

void destory_actual_trader(actual_trader*& api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
