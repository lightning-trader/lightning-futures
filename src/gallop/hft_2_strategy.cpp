#include "hft_2_strategy.h"
#include "time_utils.hpp"


void hft_2_strategy::on_init()
{
	subscribe(_code);
	
}

void hft_2_strategy::on_ready() 
{
	uint32_t trading_day = get_trading_day();
	_coming_to_close = make_datetime(trading_day, "14:58:00");
	_history_ma = 0;
	_history_price.clear();
	/*
	const position_info& pos = get_position(_code);
	if (pos.yestoday_long.usable() > 0)
	{
		double_t sell_price = std::round(pos.yestoday_long.price * (1 + _open_delta * pos.yestoday_long.usable() / 3 ));
		_yestoday_sell_order = sell_for_close(_code, pos.yestoday_long.usable() , sell_price);
	}
	if (pos.yestoday_short.usable() > 0)
	{
		double_t buy_price = std::round(pos.yestoday_short.price * (1 - _open_delta * pos.yestoday_short.usable() / 3));
		_yestoday_buy_order = buy_for_close(_code, pos.yestoday_short.usable(), buy_price);
	}
	*/
}

void hft_2_strategy::on_tick(const tick_info& tick)
{
	_last_tick = tick ; 
	add_to_history(tick.price);
	if (!is_trading_ready())
	{
		LOG_DEBUG("is_trading_ready not ready %s\n", tick.id.get_id());
		return;
	}
	if (tick.time > _coming_to_close)
	{
		LOG_DEBUG("time > _coming_to_close %s %d %d\n", tick.id.get_id(), tick.time, _coming_to_close);
		return;
	}
	LOG_TRACE("on_tick time : %d.%d %s %f %llu %llu\n", tick.time,tick.tick,tick.id.get_id(), tick.price, _buy_order, _sell_order);
	if(_buy_order != INVALID_ESTID || _sell_order != INVALID_ESTID)
	{
		LOG_TRACE("_buy_order or _sell_order not null  %s %llu %llu\n", tick.id.get_id(), _buy_order, _sell_order);
		return ;
	}
	int32_t direction = 0 ;
	if(tick.price > tick.standard+15)
	{
		direction = 1;
	}
	else if(tick.price < tick.standard-15)
	{
		direction = -1;
	}
	//direction = 1;
	const position_info& pos = get_position(tick.id);
	double_t delta = std::round(tick.standard * _open_delta);
	double_t ma_delta = tick.price - std::round(_history_ma);
	double_t buy_price = tick.buy_price() -  delta - direction * (ma_delta)+_random(_random_engine);
	double_t sell_price = tick.sell_price() +  delta + direction * (ma_delta)-_random(_random_engine);
	uint32_t sell_once = 1;
	if (pos.yestoday_long.usable() > 0)
	{
		sell_once = pos.yestoday_long.usable() > 5 ? 5: pos.yestoday_long.usable();
		sell_price += sell_once * (1+delta) / 2;
	}
	uint32_t buy_once = 1;
	if (pos.yestoday_short.usable() > 0)
	{
		buy_once = pos.yestoday_short.usable() > 5 ? 5 : pos.yestoday_short.usable();
		buy_price -= buy_once * (1+delta) / 2;
	}
	buy_price = buy_price < tick.buy_price() - _protection ? buy_price : tick.buy_price() - _protection;
	sell_price = sell_price > tick.sell_price() + _protection ? sell_price : tick.sell_price() + _protection;

	if(tick.price >= tick.standard)
	{
		if (buy_price > tick.low_limit)
		{
			_buy_order = buy_for_open(tick.id, buy_once, buy_price);
		}
		if (sell_price < tick.high_limit)
		{
			_sell_order = sell_for_open(tick.id, sell_once, sell_price);
		}
	}
	else
	{
		if (sell_price < tick.high_limit)
		{
			_sell_order = sell_for_open(tick.id, sell_once, sell_price);
		}
		if (buy_price > tick.low_limit)
		{
			_buy_order = buy_for_open(tick.id, buy_once, buy_price);
		}
	}
	
}



void hft_2_strategy::on_entrust(const order_info& order)
{
	LOG_DEBUG("on_entrust : %llu\n", order.est_id);
	if (_last_tick.time > _coming_to_close)
	{
		return;
	}
	if (order.est_id == _buy_order || order.est_id == _sell_order|| order.est_id == _yestoday_buy_order|| order.est_id == _yestoday_sell_order)
	{
		double_t current_price = _last_tick.price;
		set_cancel_condition(order.est_id, [this, current_price](const tick_info& tick)->bool {

			if (tick.time > _coming_to_close)
			{
				return true;
			}
			return false;
			});
		//set_cancel_condition(localid, std::make_shared<time_out_cdt>(get_last_time() + _cancel_seconds));
	}
	
}

void hft_2_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	LOG_DEBUG("on_trade : %llu\n", localid);
	if(localid == _buy_order)
	{
		cancel_order(_sell_order);
		_buy_order = INVALID_ESTID;
	}
	if(localid == _sell_order)
	{
		cancel_order(_buy_order);
		_sell_order = INVALID_ESTID;
	}
	if (localid == _yestoday_buy_order)
	{
		_yestoday_buy_order = INVALID_ESTID;
	}
	if (localid == _yestoday_sell_order)
	{
		_yestoday_sell_order = INVALID_ESTID;
	}
}

void hft_2_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume,uint32_t total_volume)
{
	LOG_DEBUG("on_cancel : %llu\n", localid);
	
	if(localid == _buy_order)
	{
		_buy_order = INVALID_ESTID;
	}
	if (localid == _sell_order)
	{
		_sell_order = INVALID_ESTID;
	}
	if (localid == _yestoday_buy_order)
	{
		_yestoday_buy_order = INVALID_ESTID;
	}
	if (localid == _yestoday_sell_order)
	{
		_yestoday_sell_order = INVALID_ESTID;
	}

}

void hft_2_strategy::on_error(error_type type, estid_t localid, const uint32_t error)
{
	if(type != ET_PLACE_ORDER)
	{
		return ;
	}
	if (localid == _buy_order)
	{
		_buy_order = INVALID_ESTID;
	}
	if (localid == _sell_order)
	{
		_sell_order = INVALID_ESTID;
	}
	if (localid == _yestoday_buy_order)
	{
		_yestoday_buy_order = INVALID_ESTID;
	}
	if (localid == _yestoday_sell_order)
	{
		_yestoday_sell_order = INVALID_ESTID;
	}
}

void hft_2_strategy::add_to_history(double_t price)
{
	_history_price.emplace_back(price);
	if(_history_price.size() < _history_count)
	{
		double_t sum = .0F;
		for(auto it : _history_price)
		{
			sum += it;
		}
		_history_ma = std::round(sum / _history_price.size());
		
	}
	else
	{
		double_t frist = _history_price.front();
		double_t delta = (price - frist)/ _history_count;
		_history_ma = (delta + _history_ma);
		_history_price.pop_front();
	}
	

}
