#pragma once
#include "market_api.h"
#include "trader_api.h"


class simulator : public market_api, public trader_api,public event_source<4>
{
public:
	
	virtual void play(uint32_t tradeing_day) = 0;
};