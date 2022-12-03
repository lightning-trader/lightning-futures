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
}

void hft_2_strategy::on_tick(const tick_info& tick)
{
	_last_tick = tick ; 
	add_to_history(tick.price);
	//LOG_INFO("on_tick time : %d tick : %d\n", tick->time,tick->tick);
	if(_buy_order != INVALID_ESTID || _sell_order != INVALID_ESTID)
	{
		return ;
	}
	if (tick.time > _coming_to_close)
	{
		return;
	}
	if(!is_trading_ready())
	{
		return;
	}
	double_t ma_delta = tick.price - _history_ma ;
	double_t buy_price = tick.buy_price() - _open_delta - ma_delta;
	double_t sell_price = tick.sell_price() + _open_delta - ma_delta;
	buy_price = buy_price < tick.buy_price()? buy_price : tick.buy_price();
	sell_price = sell_price > tick.sell_price() ? sell_price : tick.sell_price();
	if(buy_price > tick.low_limit)
	{
		_buy_order = buy_for_open(tick.id, 1, buy_price);
	}
	if(sell_price < tick.high_limit)
	{
		_sell_order = sell_for_open(tick.id, 1, sell_price);
	}
}



void hft_2_strategy::on_entrust(const order_info& order)
{
	LOG_DEBUG("on_entrust : %llu\n", order.est_id);
	if (_last_tick.time > _coming_to_close)
	{
		return;
	}
	if (order.est_id == _buy_order || order.est_id == _sell_order)
	{
		set_cancel_condition(order.est_id, [this](const tick_info& tick)->bool {

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

}

void hft_2_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume,uint32_t total_volume)
{
	LOG_DEBUG("on_cancel : %llu\n", localid);

	if(localid == _buy_order)
	{
		_buy_order = INVALID_ESTID;
		return ;
	}
	if (localid == _sell_order)
	{
		_sell_order = INVALID_ESTID;
		return;
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
		return;
	}
	if (localid == _sell_order)
	{
		_sell_order = INVALID_ESTID;
		return;
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
		_history_ma = std::round(delta + _history_ma);
		_history_price.pop_front();
	}
	

}
