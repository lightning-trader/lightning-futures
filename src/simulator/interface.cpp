#include "market_simulator.h"
#include "trader_simulator.h"
#include <interface.h>

dummy_market* create_dummy_market(const params& config)
{
	return new market_simulator(config);
}

void destory_dummy_market(dummy_market*& api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
dummy_trader* create_dummy_trader(const params& config)
{
	return new trader_simulator(config);
}

void destory_dummy_trader(dummy_trader*& api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
