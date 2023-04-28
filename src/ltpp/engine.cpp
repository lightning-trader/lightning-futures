#include <runtime_engine.h>
#include "strategy_manager.h"
#include <thread>

void engine::regist_strategy(straid_t id,std::shared_ptr<strategy> stra)
{
	if(_strategy_manager&&stra)
	{
		_strategy_manager->regist_strategy(id,stra);
	}
}

void engine::unregist_strategy(straid_t id)
{
	if (_strategy_manager)
	{
		_strategy_manager->unregist_strategy(id);
	}
}

void engine::set_trading_filter(filter_function callback)
{
	engine::_filter_function = callback;
	lt_set_trading_filter(_lt, engine::_filter_callback);
}


const order_statistic& engine::get_order_statistic()const
{
	return lt_get_order_statistic(_lt);
}

const position_info& engine::get_position(const code_t& code) const
{
	return lt_get_position(_lt, code);
}
