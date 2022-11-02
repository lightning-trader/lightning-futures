#include "rl_env.h"
#include "driver.h"
#include <market_api.h>
#include <trader_api.h>
#include <recorder.h>


rl_env::rl_env(driver& driver) :
	_is_runing(false),
	_driver(driver)
{
	_market = _driver.get_market_api();
	
	_trader = _driver.get_trader_api();
	
	
}
rl_env::~rl_env()
{

	if (_market)
	{
		_market = nullptr;
	}
	if (_trader)
	{
		_trader = nullptr;
	}
}



void rl_env::step()
{
/*
	auto tick_info = _market->pop_tick_info();
	if (tick_info)
	{
		//_strategy.on_tick(tick_info);
	}
	//_market->update();
	//_trader->handle_event();
*/
}


void rl_env::on_trigger(event_type type)
{
	switch (type)
	{
	case ET_CrossDay:
		
		break;
	}
}
