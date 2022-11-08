#include "dm_strategy.h"
#include <log_wapper.hpp>

void dm_strategy::on_init()
{
	set_trading_optimize(1, TO_OPEN_TO_CLOSE,false);
	subscribe({"SHFF.rb2210"});
	//add_condition(std::make_shared<fall_back_cds>());
}

void dm_strategy::on_tick(const tick_info* tick)
{

	//LOG_INFO("on_tick time : %d tick : %d\n", tick->time,tick->tick);
	if (estid_t() != _buy_order|| estid_t() != _sell_order)
	{
		return ;
	}
	const position_info* pos = get_position("SHFF.rb2210");
	if(pos==nullptr)
	{
		_buy_order = buy_for_open(tick->id, 1, tick->buy_price());
	}
	else
	{
		if(pos->long_postion == 0)
		{
			_buy_order = buy_for_open(tick->id, 1, tick->buy_price());
		}
		else if (pos->short_postion == 0)
		{
			_sell_order = sell_for_open(tick->id, 1, tick->sell_price());
		}
	}

}



void dm_strategy::on_entrust(estid_t localid)
{
	const order_info* order = get_order(localid);
	if(order == nullptr)
	{
		return ;
	}
	if(_buy_order == localid)
	{
		set_cancel_condition(localid, [order](const tick_info* tick)->bool{
			return tick->buy_price() > order->price;
		});
	}
	else
	{
		set_cancel_condition(localid, [order](const tick_info* tick)->bool {
			return tick->sell_price() > order->price;
			});
	}
	
	LOG_INFO("on_entrust tick : %s\n", localid.to_str());
}

void dm_strategy::on_trade(estid_t localid, code_t code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	if(localid == _buy_order)
	{
		_buy_order = estid_t();
		return ;
	}
	if (localid == _sell_order)
	{
		_sell_order = estid_t();
		return;
	}

}

void dm_strategy::on_cancel(estid_t localid,code_t code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	LOG_INFO("on_cancel tick : %s\n", localid.to_str());
	if (localid == _buy_order)
	{
		_buy_order = estid_t();
		return;
	}
	if (localid == _sell_order)
	{
		_sell_order = estid_t();
		return;
	}
}

