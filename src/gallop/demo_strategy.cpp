#include "demo_strategy.h"
#include <log_wapper.hpp>

void demo_strategy::on_init()
{
	set_trading_optimize(2, TO_INVALID,false);
	subscribe({"SHFF.rb2301"});
}

void demo_strategy::on_tick(const tick_info* tick)
{
	check_lose(tick);
	//LOG_INFO("on_tick time : %d tick : %d\n", tick->time,tick->tick);
	if(_buy_order.is_valid()|| _sell_order.is_valid())
	{
		return ;
	}

	_buy_order = buy_for_open(tick->id, 1, tick->buy_price()- _open_delta);
	_sell_order = sell_for_open(tick->id, 1, tick->sell_price()+ _open_delta);

}



void demo_strategy::on_entrust(estid_t localid)
{
	//set_cancel_condition(localid, std::make_shared<time_out_cdt>(get_last_time() + 60));
	LOG_INFO("on_entrust tick : %s\n", localid.to_str());
}

void demo_strategy::on_trade(estid_t localid, code_t code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	if(localid == _buy_order)
	{
		cancel_order(_sell_order);
	}
	if(localid == _sell_order)
	{
		cancel_order(_buy_order);
	}
	auto pos = get_position(code);
	if(pos && pos->is_mepty())
	{
		_buy_order = estid_t();
		_sell_order = estid_t();
	}
}

void demo_strategy::on_cancel(estid_t localid,code_t code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	LOG_INFO("on_cancel tick : %s\n", localid.to_str());
	if(localid == _buy_order)
	{
		_buy_order = estid_t();
	}
	if (localid == _sell_order)
	{
		_sell_order = estid_t();
	}
}


bool demo_strategy::check_lose(const tick_info* tick)
{
	//Ö¹Ëð
	auto position = get_position(tick->id);
	if (position == nullptr)
	{
		return false;
	}
	if (position->long_postion > 0)
	{
		if (tick->price < _long_lose_price)
		{
			sell_for_close(tick->id, position->long_postion);
			_last_order_time = tick->time;
		}
		if (_long_lose_price < tick->price - _lose_offset)
		{
			_long_lose_price = tick->price - _lose_offset;
		}
		return true;
	}
	if (position->short_postion > 0)
	{
		//³ÖÓÐ¿Õµ¥
		if (tick->price > _short_lose_price)
		{
			buy_for_close(tick->id, position->short_postion);
			_last_order_time = tick->time;
		}
		if (_short_lose_price > tick->price + _lose_offset)
		{
			_short_lose_price = tick->price + _lose_offset;
		}
		return true;
	}
	return false;
}
