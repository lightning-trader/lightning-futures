#include <runtime_engine.h>
#include "strategy_manager.h"
#include <thread>

void engine::add_strategy(straid_t id,std::shared_ptr<strategy> stra)
{
	if(_strategy_manager&&stra)
	{
		_strategy_manager->regist_strategy(id,stra);
	}
}


void engine::set_trading_optimize(uint32_t max_position, trading_optimal opt, bool flag)
{

	lt_set_trading_optimize(_lt, max_position, opt, flag);
}

void engine::set_trading_filter(std::function<bool(offset_type offset, direction_type direction)> callback)
{
	engine::_filter_function = callback;
	lt_set_trading_filter(_lt, engine::_filter_callback);
}


const order_statistic& engine::get_order_statistic()const
{
	return lt_get_order_statistic(_lt);
}