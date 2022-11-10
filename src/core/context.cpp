#include "context.h"
#include <file_wapper.hpp>
#include <driver.h>
#include <strategy.h>
#include <market_api.h>
#include <trader_api.h>
#include <recorder.h>
#include <platform_helper.hpp>
#include <log_wapper.hpp>


context::context(driver& driver, strategy& strategy):
	_is_runing(false),
	_driver(driver), 
	_strategy(strategy)
{
	_market = _driver.get_market_api();
	_trader = _driver.get_trader_api();
	_strategy.init(_market, _trader);
	_driver.add_handle(std::bind(&context::on_event_trigger, this, std::placeholders::_1, std::placeholders::_2));
}
context::~context()
{
	if(_market)
	{
		_market = nullptr ;
	}
	if (_trader)
	{
		_trader = nullptr;
	}
}


void context::start()
{
	_is_runing = true;
	this->run();
}

void context::stop()
{
	_is_runing = false ;
}


void context::run()
{
	int core = platform_helper::get_cpu_cores();
	if (!platform_helper::bind_core(core - 1))
	{
		LOG_ERROR("Binding to core {%d} failed", core);
	}
	while (_is_runing)
	{
		std::vector<const tick_info*> result;
		_market->pop_tick_info(result);
		for (auto& it : result)
		{
			_strategy.tick(it);
		}
		_driver.update();
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
}


void context::on_event_trigger(event_type type,const std::vector<std::any>& param)
{
	switch(type)
	{
		case ET_CrossDay:
			_trader->submit_settlement();
			//_strategy.on_cross_day(_market->get_trading_day());
		break;
		case ET_AccountChange:
		break;
		case ET_BeginTrading:
		break;
		case ET_EndTrading:
			{
				_is_runing = false ;
				auto acc = _trader->get_account();
				LOG_INFO("ET_EndTrading %f %f", acc->money, acc->frozen_monery);
			}
		break;
		case ET_OrderCancel:
			_strategy.cancel(param);
		break;
		case ET_OrderPlace:
			//LOG_DEBUG("on_event_trigger::ET_OrderPlace %d \n", std::any_cast<bool>(param[0]));
			_strategy.entrust(param);
		break;
		case ET_OrderDeal:
			_strategy.deal(param);
		break;
		case ET_OrderTrade:
			_strategy.trade(param);
			
			break;
		
	}	
}
