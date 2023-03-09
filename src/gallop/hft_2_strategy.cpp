#include "hft_2_strategy.h"
#include "time_utils.hpp"

void hft_2_strategy::on_init()
{
	subscribe(_code);
	use_custom_chain(TO_OPEN_TO_CLOSE,false);
}

void hft_2_strategy::on_ready() 
{
	uint32_t trading_day = get_trading_day();
	_coming_to_close = make_datetime(trading_day, "14:58:00");
}

void hft_2_strategy::on_tick(const tick_info& tick)
{
	_last_tick = tick ; 
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
	//LOG_INFO("on_tick time : %d.%d %s %f %llu %llu\n", tick.time,tick.tick,tick.id.get_id(), tick.price, _buy_order, _sell_order);
	if(_buy_order != INVALID_ESTID || _sell_order != INVALID_ESTID)
	{
		//LOG_INFO("_buy_order or _sell_order not null  %s %llu %llu\n", tick.id.get_id(), _buy_order, _sell_order);
		return ;
	}
	const position_info& pos = get_position(tick.id);
	double_t delta = (tick.standard * _open_delta);
	double_t buy_price = tick.buy_price() -  delta - _random(_random_engine);
	double_t sell_price = tick.sell_price() +  delta + _random(_random_engine);
	uint32_t sell_once = _open_once;
	uint32_t yestoday_once = _yestoday_multiple * _open_once;
	if (pos.yestoday_long.usable() > 0)
	{
		if (pos.get_long_position() > _yestoday_threshold)
		{
			yestoday_once = static_cast<uint32_t>(_yestoday_multiple * _open_once * _yestoday_growth);
		}
		sell_once = pos.yestoday_long.usable() > yestoday_once ? yestoday_once : pos.yestoday_long.usable();
		sell_price += (std::ceil(sell_once / _open_once) - 1) * delta / 2 ;
	}
	uint32_t buy_once = _open_once;
	if (pos.yestoday_short.usable() > 0)
	{
		if (pos.get_short_position() > _yestoday_threshold)
		{
			yestoday_once = static_cast<uint32_t>(_yestoday_multiple * _open_once * _yestoday_growth);
		}
		buy_once = pos.yestoday_short.usable() > yestoday_once ? yestoday_once : pos.yestoday_short.usable();
		buy_price -= (std::ceil(buy_once / _open_once) - 1) * delta / 2 ;
	}
	buy_price = std::round(buy_price);
	sell_price = std::round(sell_price);
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
	LOG_INFO("hft_2_strategy on_entrust : %llu\n", order.est_id);
	if (_last_tick.time > _coming_to_close)
	{
		return;
	}
	if (order.est_id == _buy_order || order.est_id == _sell_order)
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
	LOG_INFO("hft_2_strategy on_trade : %llu\n", localid);
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
}

void hft_2_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume,uint32_t total_volume)
{
	LOG_INFO("hft_2_strategy on_cancel : %llu\n", localid);
	
	if(localid == _buy_order)
	{
		_buy_order = INVALID_ESTID;
	}
	if (localid == _sell_order)
	{
		_sell_order = INVALID_ESTID;
	}
}

void hft_2_strategy::on_error(error_type type, estid_t localid, const uint32_t error)
{
	LOG_INFO("hft_2_strategy on_error : %llu %d\n", localid, error);
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
}
