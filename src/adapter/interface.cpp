#include <interface.h>
#include "market/ctp_api_market.h"
#include "trader/ctp_api_trader.h"

actual_market* create_actual_market(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map,const params& config)
{
	auto market_type = config.get<std::string>("market");
	if (market_type == "ctp_api")
	{
		return new ctp_api_market(id_excg_map, config);
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
actual_trader* create_actual_trader(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map, const params& config)
{
	auto trader_type = config.get<std::string>("trader");
	if (trader_type == "ctp_api")
	{
		return new ctp_api_trader(id_excg_map, config);
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
