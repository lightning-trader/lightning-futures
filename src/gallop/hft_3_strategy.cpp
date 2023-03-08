#include "hft_3_strategy.h"
#include "time_utils.hpp"


void hft_3_strategy::on_init()
{
	subscribe(_code);
	//use_custom_chain(TO_OPEN_TO_CLOSE, false);
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
	const position_info& pos = get_position(tick.id);

	if( pos.yestoday_long.usable() > 0)
	{
		int32_t once = _yestoday_ratio;
		if (pos.yestoday_long.usable() > _yestoday_ratio)
		{
			//Ö¹Ëð
			int32_t last = pos.yestoday_long.usable()-_yestoday_ratio;
			double_t last_price = tick.sell_price() + _delta;
			last_price += (std::ceil(last / _open_once) - 1) * _delta / 2;
			last_price = std::round(last_price);
			sell_for_close(tick.id, last, last_price);
		}
		else
		{
			once = pos.yestoday_long.usable();
		}
		
		double_t sell_price = pos.yestoday_long.price + _delta;
		if (sell_price < tick.sell_price() + _delta)
		{
			sell_price = tick.sell_price() + _delta;
		}
		sell_price = std::round(sell_price);
		sell_for_close(tick.id, once, sell_price);

	}
	if (pos.yestoday_short.usable() > 0)
	{
		int32_t once = _yestoday_ratio;
		if (pos.yestoday_short.usable() > _yestoday_ratio)
		{
			int32_t last = pos.yestoday_short.usable() - _yestoday_ratio;
			double_t last_price = tick.buy_price() - _delta;
			last_price -= (std::ceil(last / _open_once) - 1) * _delta / 2;
			last_price = std::round(last_price);
			buy_for_close(tick.id, last, last_price);
		}
		else
		{
			once = pos.yestoday_short.usable();
		}
		double_t buy_price = pos.yestoday_short.price - _delta ;
		if (buy_price > tick.buy_price() - _delta)
		{
			buy_price = tick.buy_price() - _delta;
		}
		buy_price = std::round(buy_price);
		buy_for_close(tick.id, once, buy_price);
		
	}
	

	if(_open_long_order == INVALID_ESTID)
	{
		uint32_t once = static_cast<uint32_t>(std::round(pos.get_long_position() * _beta + _open_once));

		double_t buy_price = tick.buy_price() - _random(_random_engine) - (pos.get_long_position() * _alpha / _open_once + 1) * _delta;
		buy_price = buy_price < tick.buy_price() ? buy_price : tick.buy_price() ;
		buy_price = std::round(buy_price);
		if (buy_price > tick.low_limit)
		{
			_open_long_order = buy_for_open(tick.id, once, buy_price);
		}
		
	}
	if (_open_short_order == INVALID_ESTID)
	{
		uint32_t once = static_cast<uint32_t>(std::round(pos.get_short_position() * _beta + _open_once));

		double_t sell_price = tick.sell_price() + _random(_random_engine) + (pos.get_short_position() * _alpha / _open_once + 1) * _delta;;
		sell_price = sell_price > tick.sell_price() ? sell_price : tick.sell_price() ;
		sell_price = std::round(sell_price);
		if (sell_price < tick.high_limit)
		{
			_open_short_order = sell_for_open(tick.id, once, sell_price);
		}
		
	}
	if (_close_long_order == INVALID_ESTID)
	{
		if(pos.today_long.usable() > 0)
		{
			double_t sell_price = pos.today_long.price + _random(_random_engine) + _delta;
			sell_price = sell_price > tick.sell_price() ? sell_price : tick.sell_price();
			sell_price = std::round(sell_price);
			_close_long_order = sell_for_close(tick.id, pos.today_long.usable(), sell_price);
		}
		
	}
	if (_close_short_order == INVALID_ESTID)
	{
		if(pos.today_short.usable() > 0)
		{
			double_t buy_price = pos.today_short.price - _random(_random_engine) - _delta;
			buy_price = buy_price < tick.buy_price() ? buy_price : tick.buy_price();
			buy_price = std::round(buy_price);
			_close_short_order = buy_for_close(tick.id, pos.today_short.usable(), buy_price);
		}
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

