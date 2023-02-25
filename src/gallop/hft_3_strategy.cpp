#include "hft_3_strategy.h"
#include "time_utils.hpp"


void hft_3_strategy::on_init()
{
	subscribe(_code);
	
}

void hft_3_strategy::on_ready()
{
	uint32_t trading_day = get_trading_day();
	_coming_to_close = make_datetime(trading_day, "14:58:00");
	
}

void hft_3_strategy::on_tick(const tick_info& tick)
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
	LOG_TRACE("on_tick time : %d.%d %s %f %llu %llu\n", tick.time,tick.tick,tick.id.get_id(), tick.price, _open_short_order, _open_long_order);
	uint32_t once = _open_once;
	const position_info& pos = get_position(tick.id);
	if(_open_long_order == INVALID_ESTID)
	{
		if(pos.today_long.usable()>0)
		{
			once = static_cast<uint32_t>(std::round(pos.today_long.usable() * _beta + _open_once));
			once = pos.today_long.usable() < once ? pos.today_long.usable() : once;
		}
		double_t buy_price = tick.buy_price() - _random(_random_engine) - (pos.today_long.usable() * _alpha / _open_once + _open_once) * _delta;
		buy_price = buy_price < tick.buy_price() ? buy_price : tick.buy_price() ;
		buy_price = std::round(buy_price);
		if (buy_price > tick.low_limit)
		{
			_open_long_order = buy_for_open(tick.id, once, buy_price);
		}
		
	}
	if (_open_short_order == INVALID_ESTID)
	{
		
		if (pos.today_short.usable() > 0)
		{
			once = static_cast<uint32_t>(std::round(pos.today_short.usable() * _beta + _open_once));
			once = pos.today_short.usable() < once ? pos.today_short.usable() : once;
		}
		double_t sell_price = tick.sell_price() + _random(_random_engine) + (pos.today_short.usable() * _alpha / _open_once + _open_once) * _delta;;
		sell_price = sell_price > tick.sell_price() ? sell_price : tick.sell_price() ;
		sell_price = std::round(sell_price);
		if (sell_price < tick.high_limit)
		{
			_open_short_order = sell_for_open(tick.id, once, sell_price);
		}
		
	}
	if (_close_long_order == INVALID_ESTID && pos.today_long.usable() > 0)
	{
		double_t sell_price = pos.today_long.price + _random(_random_engine) + _delta;
		sell_price = sell_price > tick.sell_price() ? sell_price : tick.sell_price();
		sell_price = std::round(sell_price);
		_close_long_order = sell_for_close(tick.id, pos.today_long.usable(), sell_price);
	}
	if (_close_short_order == INVALID_ESTID && pos.today_short.usable() > 0)
	{
		double_t buy_price = pos.today_short.price - _random(_random_engine) - _delta;
		buy_price = buy_price < tick.buy_price() ? buy_price : tick.buy_price();
		buy_price = std::round(buy_price);
		_close_short_order = buy_for_close(tick.id, pos.today_short.usable(), buy_price);
	}
	
}



void hft_3_strategy::on_entrust(const order_info& order)
{
	LOG_DEBUG("on_entrust : %llu\n", order.est_id);
	if (_last_tick.time > _coming_to_close)
	{
		return;
	}
	if (order.est_id == _open_short_order || order.est_id == _open_long_order || order.est_id == _close_long_order || order.est_id == _close_short_order )
	{
		set_cancel_condition(order.est_id, [this](const tick_info& tick)->bool {

			if (tick.time > _coming_to_close)
			{
				return true;
			}
			return false;
			});
	}
	
}

void hft_3_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	LOG_DEBUG("on_trade : %llu\n", localid);
	if(localid == _open_long_order)
	{
		cancel_order(_close_long_order);
		cancel_order(_open_short_order);
		_open_long_order = INVALID_ESTID;
	}
	if(localid == _open_short_order)
	{
		cancel_order(_close_short_order);
		cancel_order(_open_long_order);
		_open_short_order = INVALID_ESTID;
	}
	if (localid == _close_long_order)
	{
		_close_long_order = INVALID_ESTID;
	}
	if (localid == _close_short_order)
	{
		_close_short_order = INVALID_ESTID;
	}
}

void hft_3_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume,uint32_t total_volume)
{
	LOG_DEBUG("on_cancel : %llu\n", localid);
	
	if(localid == _open_long_order)
	{
		_open_long_order = INVALID_ESTID;
	}
	if (localid == _open_short_order)
	{
		_open_short_order = INVALID_ESTID;
	}
	if (localid == _close_long_order)
	{
		_close_long_order = INVALID_ESTID;	
	}
	if (localid == _close_short_order)
	{
		_close_short_order = INVALID_ESTID;
	}
}

void hft_3_strategy::on_error(error_type type, estid_t localid, const uint32_t error)
{
	if(type != ET_PLACE_ORDER)
	{
		return ;
	}
	if (localid == _open_long_order)
	{
		_open_long_order = INVALID_ESTID;
	}
	if (localid == _open_short_order)
	{
		_open_short_order = INVALID_ESTID;
	}
	if (localid == _close_long_order)
	{
		_close_long_order = INVALID_ESTID;
	}
	if (localid == _close_short_order)
	{
		_close_short_order = INVALID_ESTID;
	}
}

