#include "hcc_strategy.h"
//#include <time_util.h>

void hcc_strategy::on_init()
{
	set_trading_optimize(2, TO_OPEN_TO_CLOSE, true);
	subscribe({ _code });
}

/*
 *	tick推送
 */
void hcc_strategy::on_tick(const tick_info* tick)
{
	//LOG_INFO("tick:%f %d \n", tick->price, tick->volume);
	
	if (_order_id == INVALID_ESTID)
	{
		return;
	}

	/*
	if (check_lose(tick))
	{
		return;
	}
	*/
	if (tick->price < buy_pending_order.first)
	{
		//下跌中
		on_price_change(tick, false);
	}
	else if (tick->price > sell_pending_order.first)
	{
		//上攻中
		on_price_change(tick, true);
	}
	else
	{
		float delta_volume = static_cast<float>(tick->volume - current_volume);
		if (tick->buy_price() == tick->price)
		{
			float r = delta_volume / buy_pending_order.second;
			r = r < 1 ? 1 : r;
			short_drag += tick->total_buy_valume() * r;
		}
		else if (tick->sell_price() == tick->price)
		{
			float r = delta_volume / sell_pending_order.second;
			r = r < 1 ? 1 : r;
			long_drag += tick->total_sell_valume() * r;
		}
	}
}

bool hcc_strategy::check_lose(const tick_info* tick)
{
	//止损
	auto position = get_position(_code);

	if(position.long_postion>0)
	{
		if (tick->price < _long_lose_price)
		{
			_order_id = sell_for_close(_code, position.long_postion,tick->price);
			_last_order_time = tick->time;
		}
		if(_long_lose_price < tick->price - _lose_offset)
		{
			_long_lose_price = tick->price - _lose_offset;
		}
		return true;
	}
	if(position.short_postion > 0)
	{
		//持有空单
		if(tick->price > _short_lose_price)
		{
			_order_id = buy_for_close(_code, position.short_postion, tick->price);
			_last_order_time = tick->time;
		}
		if (_short_lose_price > tick->price + _lose_offset)
		{
			_short_lose_price = tick->price + _lose_offset;
		}
		return true;
	}
	return false ;
}


void hcc_strategy::on_price_change(const tick_info* tick, bool is_up)
{
	buy_pending_order = tick->buy_order[0];
	sell_pending_order = tick->sell_order[0];
	current_volume = tick->volume;
	//#计算理论价格
	//pxInThry = (newTick["bid_price_0"] * newTick["ask_qty_0"] + newTick["ask_price_0"] * newTick["bid_qty_0"]) / (newTick["ask_qty_0"] + newTick["bid_qty_0"])
	double px = (tick->buy_order[0].first* tick->sell_order[0].second+ tick->sell_order[0].first* tick->buy_order[0].second)/(tick->buy_order[0].second+ tick->sell_order[0].second);
	if (short_drag > _drag_limit && long_drag > _drag_limit)
	{
		if (!is_up)
		{
			if (long_drag < short_drag * _record_ratio && px< tick->price)
			{
				_order_id = buy_for_open(_code, 1, tick->price);
				_long_lose_price = tick->price - _lose_offset;
				_last_order_time = tick->time;
			}

		}
		else
		{

			if (short_drag < long_drag * _record_ratio && px > tick->price)
			{
				_order_id = sell_for_open(_code, 1, tick->price);
				_short_lose_price = tick->price - _lose_offset;
				_last_order_time = tick->time;
			}

		}

	}
	
	short_drag = 0;
	long_drag = 0;
}


void hcc_strategy::on_entrust(estid_t localid)
{
	set_cancel_condition(localid, [this](const tick_info* tick)->bool{
		return tick->time > (get_last_time() + _cancel_seconds);
	});
	_last_order_time = get_last_time();
	LOG_INFO("on_entrust success tick : %llu\n", localid);
}

void hcc_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction,double_t price,uint32_t volume)
{
	//LOG_INFO("on_trade tick : %s\n", localid.id.c_str());

	auto& acc = get_account();
	auto& pos = get_position(_code);

	LOG_INFO("on_trade %llu %f %f [%d %d %d %d]\n", localid, acc.money, acc.frozen_monery, pos.long_postion, pos.short_postion, pos.long_frozen, pos.short_frozen);
	_order_id = INVALID_ESTID;
}

void hcc_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	auto& pos = get_position(_code);
	LOG_INFO("on_cancel tick : %llu [%d %d %d %d]\n", localid, pos.long_postion, pos.short_postion, pos.long_frozen, pos.short_frozen);
	_order_id = INVALID_ESTID;
}
