#include "sig_1_strategy.h"
#include "time_utils.hpp"


void sig_1_strategy::on_init()
{
	subscribe(_code);
	if(_expire != default_code)
	{
		subscribe(_expire);
	}
	//use_custom_chain(TO_OPEN_TO_CLOSE, false);
	_order_data = static_cast<persist_data*>(get_userdata(sizeof(persist_data)));
}

void sig_1_strategy::on_ready()
{
	uint32_t trading_day = get_trading_day();
	_coming_to_close = make_datetime(trading_day, "14:58:00");
	if(_order_data->trading_day!=trading_day)
	{
		_order_data->trading_day = trading_day;
		for (size_t i = 0; i < ORDER_ESTID_COUNT; i++)
		{
			_order_data->order_estids[i] = INVALID_ESTID;
		}
	}
	else
	{
		for (size_t i = 0; i < ORDER_ESTID_COUNT; i++)
		{
			if(_order_data->order_estids[i] != INVALID_ESTID)
			{
				auto& order = get_order(_order_data->order_estids[i]);
				if (order.est_id != INVALID_ESTID)
				{
					set_cancel_condition(order.est_id, [this](const tick_info& tick)->bool {

						if (tick.time > _coming_to_close)
						{
							return true;
						}
						return false;
						});
				}
				else
				{
					_order_data->order_estids[i] = INVALID_ESTID;
				}
			}
		}
	}
}

void sig_1_strategy::on_tick(const tick_info& tick, const deal_info& deal)
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
	LOG_TRACE("on_tick time : %d.%d %s %f %llu %llu\n", tick.time,tick.tick,tick.id.get_id(), tick.price, _order_data->order_estids[CLOSE_LONG_ORDER], _order_data->order_estids[OPEN_LONG_ORDER]);
	
	if (tick.id == _expire)
	{
		const position_info& expire_pos = get_position(tick.id);
		if (expire_pos.yestoday_long.usable() > 0 && _order_data->order_estids[TRANSFER_CLOSE_LONG] == INVALID_ESTID)
		{
			//Ö¹Ëð
			//int32_t last = expire_pos.yestoday_long.usable();
			double_t last_price = tick.sell_price() + _delta;
			//last_price += (last / _open_once) * _delta / 2.F;
			//last_price = last_price < expire_pos.yestoday_long.price ? last_price : expire_pos.yestoday_long.price;
			last_price = std::round(last_price);
			_order_data->order_estids[TRANSFER_CLOSE_LONG] = sell_for_close(tick.id, _open_once, last_price);
		}
		if (expire_pos.yestoday_short.usable() > 0 && _order_data->order_estids[TRANSFER_CLOSE_SHORT] == INVALID_ESTID)
		{
			//int32_t last = expire_pos.yestoday_short.usable();
			double_t last_price = tick.buy_price() - _delta;
			//last_price -= (last / _open_once) * _delta / 2.F;
			//last_price = last_price > expire_pos.yestoday_short.price ? last_price : expire_pos.yestoday_short.price ;
			last_price = std::round(last_price);
			_order_data->order_estids[TRANSFER_CLOSE_SHORT] = buy_for_close(tick.id, _open_once, last_price);
		}
	}
	else
	{
		const position_info& pos = get_position(tick.id);
		if (pos.yestoday_long.usable() > 0)
		{

			uint32_t once = 0;
			if (pos.yestoday_long.usable() > _yestoday_ratio)
			{
				//Ö¹Ëð
				int32_t last = pos.yestoday_long.usable() - _yestoday_ratio;
				double_t last_price = tick.sell_price() + _delta;
				last_price += (last / _open_once) * _delta / 2.F;
				last_price = last_price < pos.yestoday_long.price + _delta ? last_price : pos.yestoday_long.price + _delta;
				last_price = std::round(last_price);
				_order_data->order_estids[REGARDLESS_CLOSE_LONG] = sell_for_close(tick.id, last, last_price);
				once = _yestoday_ratio;
			}
			else
			{
				once = pos.yestoday_long.usable();
			}
			if(once>0)
			{
				double_t sell_price = pos.yestoday_long.price + _delta;
				if (sell_price < tick.sell_price() + _delta)
				{
					sell_price = tick.sell_price() + _delta;
				}
				sell_price = std::round(sell_price);
				_order_data->order_estids[YESTODAY_CLOSE_LONG] = sell_for_close(tick.id, once, sell_price);
			}
			

		}
		if (pos.yestoday_short.usable() > 0)
		{

			uint32_t once = 0;
			if (pos.yestoday_short.usable() > _yestoday_ratio)
			{
				int32_t last = pos.yestoday_short.usable() - _yestoday_ratio;
				double_t last_price = tick.buy_price() - _delta;
				last_price -= (last / _open_once) * _delta / 2.F;
				last_price = last_price > pos.yestoday_short.price - _delta ? last_price : pos.yestoday_short.price - _delta;
				last_price = std::round(last_price);
				_order_data->order_estids[REGARDLESS_CLOSE_SHORT] = buy_for_close(tick.id, last, last_price);
				once = _yestoday_ratio;
			}
			else
			{
				once = pos.yestoday_short.usable();
			}
			if(once>0)
			{
				double_t buy_price = pos.yestoday_short.price - _delta;
				if (buy_price > tick.buy_price() - _delta)
				{
					buy_price = tick.buy_price() - _delta;
				}
				buy_price = std::round(buy_price);
				_order_data->order_estids[YESTODAY_CLOSE_SHORT] = buy_for_close(tick.id, once, buy_price);
			}
		}


		if (_order_data->order_estids[OPEN_LONG_ORDER] == INVALID_ESTID)
		{
			uint32_t once = static_cast<uint32_t>(std::round(pos.get_long_position() * _beta + _open_once));

			double_t buy_price = tick.buy_price() - _random(_random_engine) - (pos.get_long_position() * _alpha / _open_once + 1) * _delta;
			buy_price = buy_price < tick.buy_price() ? buy_price : tick.buy_price();
			buy_price = std::round(buy_price);
			if (buy_price > tick.low_limit)
			{
				_order_data->order_estids[OPEN_LONG_ORDER] = buy_for_open(tick.id, once, buy_price);
			}

		}
		if (_order_data->order_estids[OPEN_SHORT_ORDER] == INVALID_ESTID)
		{
			uint32_t once = static_cast<uint32_t>(std::round(pos.get_short_position() * _beta + _open_once));

			double_t sell_price = tick.sell_price() + _random(_random_engine) + (pos.get_short_position() * _alpha / _open_once + 1) * _delta;;
			sell_price = sell_price > tick.sell_price() ? sell_price : tick.sell_price();
			sell_price = std::round(sell_price);
			if (sell_price < tick.high_limit)
			{
				_order_data->order_estids[OPEN_SHORT_ORDER] = sell_for_open(tick.id, once, sell_price);
			}

		}
		if (_order_data->order_estids[CLOSE_LONG_ORDER] == INVALID_ESTID)
		{
			if (pos.today_long.usable() > 0)
			{
				double_t sell_price = pos.today_long.price + _random(_random_engine) + _delta;
				sell_price = sell_price > tick.sell_price() ? sell_price : tick.sell_price();
				sell_price = std::round(sell_price);
				_order_data->order_estids[CLOSE_LONG_ORDER] = sell_for_close(tick.id, pos.today_long.usable(), sell_price);
			}

		}
		if (_order_data->order_estids[CLOSE_SHORT_ORDER] == INVALID_ESTID)
		{
			if (pos.today_short.usable() > 0)
			{
				double_t buy_price = pos.today_short.price - _random(_random_engine) - _delta;
				buy_price = buy_price < tick.buy_price() ? buy_price : tick.buy_price();
				buy_price = std::round(buy_price);
				_order_data->order_estids[CLOSE_SHORT_ORDER] = buy_for_close(tick.id, pos.today_short.usable(), buy_price);
			}
		}
	}
	
	
}



void sig_1_strategy::on_entrust(const order_info& order)
{
	LOG_INFO("emg_2_strategy on_entrust : %llu %s %d %d %f %d/%d\n", order.est_id, order.code,order.direction,order.offset,order.price, order.last_volume,order.total_volume);
	if (_last_tick.time > _coming_to_close)
	{
		return;
	}

	for (size_t i = 0; i < ORDER_ESTID_COUNT; i++)
	{
		if(_order_data->order_estids[i] == order.est_id)
		{
			set_cancel_condition(order.est_id, [this](const tick_info& tick)->bool {

				if (tick.time > _coming_to_close)
				{
					return true;
				}
				return false;
				});
			break;
		}
	}

}

void sig_1_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	LOG_INFO("emg_2_strategy on_trade : %llu %s %d %d %f %d\n", localid, code, direction, offset, price, volume);
	if(localid == _order_data->order_estids[OPEN_LONG_ORDER])
	{
		cancel_order(_order_data->order_estids[CLOSE_LONG_ORDER]);
		cancel_order(_order_data->order_estids[OPEN_SHORT_ORDER]);
	}
	if(localid == _order_data->order_estids[OPEN_SHORT_ORDER])
	{
		cancel_order(_order_data->order_estids[CLOSE_SHORT_ORDER]);
		cancel_order(_order_data->order_estids[OPEN_LONG_ORDER]);
	}
	for (size_t i = 0; i < ORDER_ESTID_COUNT; i++)
	{
		if (_order_data->order_estids[i] == localid)
		{
			_order_data->order_estids[i] = INVALID_ESTID;
			break;
		}
	}
}

void sig_1_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume,uint32_t total_volume)
{
	LOG_INFO("emg_2_strategy on_cancel : %llu %s %d %d %f %d\n", localid, code, direction, offset, price, cancel_volume);
	for (size_t i = 0; i < ORDER_ESTID_COUNT; i++)
	{
		if (_order_data->order_estids[i] == localid)
		{
			_order_data->order_estids[i] = INVALID_ESTID;
			break;
		}
	}
}

void sig_1_strategy::on_error(error_type type, estid_t localid, const uint32_t error)
{
	LOG_ERROR("emg_2_strategy on_error : %llu %d \n", localid, error);
	if(type != ET_PLACE_ORDER)
	{
		return ;
	}
	for (size_t i = 0; i < ORDER_ESTID_COUNT; i++)
	{
		if (_order_data->order_estids[i] == localid)
		{
			_order_data->order_estids[i] = INVALID_ESTID;
			break;
		}
	}
}
void sig_1_strategy::on_destory()
{
	unsubscribe(_code);
	if (_expire != default_code)
	{
		unsubscribe(_expire);
	}
}
