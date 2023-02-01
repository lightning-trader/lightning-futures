#include "demo_strategy.h"

void demo_strategy::on_init()
{
	subscribe({"SHFF.ag2212"});
	//add_condition(std::make_shared<fall_back_cds>());
}

void demo_strategy::on_tick(const tick_info& tick)
{

	//LOG_INFO("on_tick time : %d tick : %d\n", tick->time,tick->tick);
	if (INVALID_ESTID != _buy_order|| INVALID_ESTID != _sell_order)
	{
		return ;
	}

	_buy_order = buy_for_open(tick.id, 1, tick.buy_price()- _open_delta);
	_sell_order = sell_for_open(tick.id, 1, tick.sell_price()+ _open_delta);

}



void demo_strategy::on_entrust(const order_info& order)
{
	//set_cancel_condition(localid, std::make_shared<time_out_cdt>(get_last_time() + 60));
	LOG_INFO("on_entrust tick : %llu\n", order.est_id);
}

void demo_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	if(localid == _buy_order)
	{
		_buy_order = INVALID_ESTID;
		cancel_order(_sell_order);
	}
	if(localid == _sell_order)
	{
		_sell_order = INVALID_ESTID;
		cancel_order(_buy_order);
	}
	if(offset == OT_OPEN)
	{
		if(direction == DT_LONG)
		{
			_highest_price = price;
		}
		else if(direction == DT_SHORT)
		{
			_lowest_price = price;
		}
	}
}

void demo_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	LOG_INFO("on_cancel tick : %llu\n", localid);
	if (localid == _buy_order)
	{
		_buy_order = INVALID_ESTID;
	
	}
	if (localid == _sell_order)
	{
		_sell_order = INVALID_ESTID;
	
	}
}


