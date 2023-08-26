#include <interface.h>
#include "ctp_trader.h"

actual_trader* create_actual_trader(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map, const params& config)
{
	auto trader_type = config.get<std::string>("trader");
	if (trader_type == "ctp")
	{
		return new ctp_trader(id_excg_map, config);
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
