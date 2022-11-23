#include "hft_1_strategy.h"
#include <time_utils.hpp>

void hft_1_strategy::on_init()
{
	set_trading_optimize(2, TO_INVALID,false);
	subscribe({"SHFE.ag2212"});
}

void hft_1_strategy::on_tick(const tick_info& tick)
{
	_last_tick = tick ; 
	add_to_history(tick.price);
	_coming_to_close = make_datetime(tick.trading_day,"14:58:00");
	//LOG_INFO("on_tick time : %d tick : %d\n", tick->time,tick->tick);
	if(_buy_order != INVALID_ESTID || _sell_order != INVALID_ESTID|| _profit_order != INVALID_ESTID || _loss_order != INVALID_ESTID)
	{
		return ;
	}
	if(get_last_time() - _last_lose_time < _lose_cd_seconds)
	{
		return ;
	}
	if (tick.time > _coming_to_close)
	{
		return ;
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



void hft_1_strategy::on_entrust(const order_info& order)
{
	LOG_DEBUG("on_entrust : %llu\n", order.est_id);
	if (_last_tick.time > _coming_to_close)
	{
		return;
	}
	if (order.est_id == _buy_order|| order.est_id == _sell_order)
	{
		set_cancel_condition(order.est_id, [this](const tick_info* tick)->bool {

			if (tick->time > _coming_to_close)
			{
				return true;
			}
			return false;
			});
		//set_cancel_condition(localid, std::make_shared<time_out_cdt>(get_last_time() + _cancel_seconds));
	}else
	{
		if (order.offset == OT_CLOSE)
		{
			if (order.direction == DT_LONG)
			{
				double_t lost_price = _last_tick.price - _lose_delta;
				set_cancel_condition(order.est_id, [this, lost_price](const tick_info* tick)->bool {
					if (tick->price < lost_price)
					{
						return true;
					}
					if (tick->time > _coming_to_close)
					{
						return true;
					}
					return false;
					});
			}
			else if (order.direction == DT_SHORT)
			{
				double_t lost_price = _last_tick.price + _lose_delta;
				set_cancel_condition(order.est_id, [this, lost_price](const tick_info* tick)->bool {
					if (tick->price > lost_price)
					{
						return true;
					}
					if (tick->time > _coming_to_close)
					{
						return true;
					}
					return false;
					});
			}
		}
	}
	
	
}

void hft_1_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	LOG_DEBUG("on_trade : %llu\n", localid);
	if(localid == _buy_order)
	{
		cancel_order(_sell_order);
		_profit_order = sell_for_close(code, volume, price + _close_delta);
		_buy_order = INVALID_ESTID;
	}
	if(localid == _sell_order)
	{
		cancel_order(_buy_order);
		_profit_order = buy_for_close(code, volume, price - _close_delta);
		_sell_order = INVALID_ESTID;
	}

	if (localid == _profit_order)
	{
		_profit_order = INVALID_ESTID;
	}
	if(localid == _loss_order)
	{
		_last_lose_time = get_last_time();
		_loss_order = INVALID_ESTID;
	}
}

void hft_1_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume,uint32_t total_volume)
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
	if (localid == _profit_order || localid == _loss_order)
	{
		//止盈单被撤销，直接市价单止损
		if(localid == _profit_order)
		{
			_profit_order = INVALID_ESTID;
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
void hft_1_strategy::add_to_history(double_t price)
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
