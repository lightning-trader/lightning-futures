#include "hft_1_strategy.h"
#include <log_wapper.hpp>

void hft_1_strategy::on_init()
{
	set_trading_optimize(2, TO_INVALID,false);
	subscribe({"SHFE.rb2301"});
}

void hft_1_strategy::on_tick(const tick_info* tick)
{
	_last_tick = *tick ; 
	//LOG_INFO("on_tick time : %d tick : %d\n", tick->time,tick->tick);
	if(_buy_order.is_valid()|| _sell_order.is_valid()|| _profit_order.is_valid()|| _loss_order.is_valid())
	{
		return ;
	}
	if(get_last_time() - _last_lose_time < _lose_cd_seconds)
	{
		return ;
	}
	_buy_order = buy_for_open(tick->id, 1, tick->buy_price()- _open_delta, OF_OTK);
	_sell_order = sell_for_open(tick->id, 1, tick->sell_price() + _open_delta, OF_OTK);

}



void hft_1_strategy::on_entrust(estid_t localid)
{
	LOG_DEBUG("on_entrust : %s\n", localid.to_str());
	if (localid == _buy_order|| localid == _sell_order)
	{
		//set_cancel_condition(localid, std::make_shared<time_out_cdt>(get_last_time() + _cancel_seconds));
	}
	else
	{
		//set_cancel_condition(localid, std::make_shared<time_out_cdt>(get_last_time() + _cancel_seconds));
		auto order = get_order(localid);
		if(order && order->offset == OT_CLOSE)
		{
			if(order->direction == DT_LONG)
			{
				double_t lost_price = _last_tick.price - _lose_delta;
				set_cancel_condition(localid, [lost_price](const tick_info* tick)->bool {
					return tick->price < lost_price;
				});
			}
			else if(order->direction == DT_SHORT)
			{
				double_t lost_price = _last_tick.price + _lose_delta;
				set_cancel_condition(localid, [lost_price](const tick_info* tick)->bool {
					return tick->price > lost_price;
					});
			}
		}
	}
	
}

void hft_1_strategy::on_trade(estid_t localid, code_t code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	LOG_DEBUG("on_trade : %s\n", localid.to_str());
	if(localid == _buy_order)
	{
		cancel_order(_sell_order);
		_profit_order = sell_for_close(code, volume, price + _close_delta);
		_buy_order = estid_t();
	}
	if(localid == _sell_order)
	{
		cancel_order(_buy_order);
		_profit_order = buy_for_close(code, volume, price - _close_delta);
		_sell_order = estid_t();
	}

	if (localid == _profit_order)
	{
		_profit_order = estid_t();
	}
	if(localid == _loss_order)
	{
		_last_lose_time = get_last_time();
		_loss_order = estid_t();
	}
}

void hft_1_strategy::on_cancel(estid_t localid,code_t code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume,uint32_t total_volume)
{
	LOG_DEBUG("on_cancel : %s\n", localid.to_str());

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
	if (localid == _profit_order || localid == _loss_order)
	{
		//止盈单被撤销，直接市价单止损
		if(localid == _profit_order)
		{
			_profit_order = estid_t();
		}
		if(offset == OT_CLOSE)
		{
			if(direction == DT_LONG)
			{
				_loss_order = sell_for_close(code, cancel_volume);
			}
			if (direction == DT_SHORT)
			{
				_loss_order = buy_for_close(code, cancel_volume);
			}
		}
		return ;
	}
	
}
