#include "demo_strategy.h"
#include <log_wapper.hpp>

void demo_strategy::on_init()
{
	set_trading_optimize(2, TO_INVALID,false);
	subscribe({"SHFF.ag2212"});
	//add_condition(std::make_shared<fall_back_cds>());
}

void demo_strategy::on_tick(const tick_info* tick)
{
	if(check_lose(tick))
	{
		return ;
	}
	//LOG_INFO("on_tick time : %d tick : %d\n", tick->time,tick->tick);
	if (INVALID_ESTID != _buy_order|| INVALID_ESTID != _sell_order)
	{
		return ;
	}

	_buy_order = buy_for_open(tick->id, 1, tick->buy_price()- _open_delta);
	_sell_order = sell_for_open(tick->id, 1, tick->sell_price()+ _open_delta);

}



void demo_strategy::on_entrust(estid_t localid)
{
	//set_cancel_condition(localid, std::make_shared<time_out_cdt>(get_last_time() + 60));
	LOG_INFO("on_entrust tick : %llu\n", localid);
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


bool demo_strategy::check_lose(const tick_info* tick)
{
	//Ö¹Ëð
	auto& position = get_position(tick->id);

	if (position.long_postion > 0)
	{
		if (_highest_price < tick->price)
		{
			_highest_price = tick->price;
		}
		if (tick->price < _highest_price - _lose_offset)
		{
			sell_for_close(tick->id, position.long_postion);
		}

		return true;
	}
	if (position.short_postion > 0)
	{
		//³ÖÓÐ¿Õµ¥
		if (_lowest_price > tick->price)
		{
			_lowest_price = tick->price;
		}
		if (tick->price > _lowest_price + _lose_offset)
		{
			buy_for_close(tick->id, position.short_postion);
		}
		return true;
	}
	return false;
}
