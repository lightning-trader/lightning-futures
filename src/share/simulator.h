#pragma once
#include "market_api.h"
#include "trader_api.h"


class simulator : public market_api, public trader_api,public event_source
{
public:


	virtual void set_trading_day(uint32_t tradeing_day) = 0 ;

	virtual void play() = 0;
};