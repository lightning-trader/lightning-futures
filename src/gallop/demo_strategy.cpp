#include "demo_strategy.h"

void demo_strategy::on_init()
{
	// 这里设置开平互转，开仓->平仓（开多->平空，开空->平多）
	set_trading_optimize(16, TO_OPEN_TO_CLOSE,false);
	subscribe({"SHFF.rb2307"});
	set_trading_filter([this](offset_type offset, direction_type direction)->bool{
		const auto & statius = get_order_statistic();
		//限制每日下单数量超过480就不再可以下单了，避免触发交易所警告
		if(statius.entrust_amount > 480)
		{	
			return false ;
		}
		return true ;
	});
}

void demo_strategy::on_tick(const tick_info& tick)
{
	_last_tick = tick;
	LOG_DEBUG("on_tick time : %d tick : %d\n", tick.time,tick.tick);
	//一轮还没有进行完，不能开启下一轮
	if (INVALID_ESTID != _short_order || INVALID_ESTID != _long_order)
	{
		return ;
	}
	//触发跌停不做
	if(tick.buy_price()<tick.low_limit)
	{
		return ;
	}
	//触发涨停也不做
	if (tick.sell_price() > tick.high_limit)
	{
		return;
	}
	// 同时挂出买卖单（因为设置了开平互转，所以不需要关心平仓问题，只需要无脑开仓）
	_long_order = buy_for_open(tick.id, 8, tick.buy_price());
	_short_order = sell_for_open(tick.id, 8, tick.sell_price());

}



void demo_strategy::on_entrust(const order_info& order)
{
	//
	double_t order_price = order.price ;
	set_cancel_condition(order.est_id, [this, order_price](const tick_info& tick)->bool {

		if (order_price != tick.buy_price()&& order_price != tick.sell_price())
		{
			return true;
		}
		return false;
	});
	LOG_INFO("on_entrust tick : %llu\n", order.est_id);
}

void demo_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	//成交以后记得把_long_order和_short_order设置成NULL这样才可以进行下一轮
	if(localid == _long_order)
	{
		_long_order = INVALID_ESTID;
	}
	if(localid == _short_order)
	{
		_short_order = INVALID_ESTID;
	}
	
}

void demo_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	LOG_INFO("on_cancel tick : %llu\n", localid);
	if (localid == _long_order)
	{
		_long_order = place_order(offset, direction, code, cancel_volume, _last_tick.buy_price());
	}
	if (localid == _short_order)
	{
		_short_order = place_order(offset, direction, code, cancel_volume, _last_tick.sell_price());
	}
}
