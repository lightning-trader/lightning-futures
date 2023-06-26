#include "marketing_strategy.h"
#include "time_utils.hpp"

using namespace lt;


void marketing_strategy::on_init(subscriber& suber)
{
	suber.regist_tick_receiver(_code,this);

	use_custom_chain(false);
	_order_data = static_cast<persist_data*>(get_userdata(sizeof(persist_data)));
}

void marketing_strategy::on_ready()
{
	uint32_t trading_day = get_trading_day();
	_coming_to_close = make_datetime(trading_day, "14:58:00");
	if (_order_data->trading_day != trading_day)
	{
		_order_data->trading_day = trading_day;
		_order_data->buy_order = INVALID_ESTID;
		_order_data->sell_order = INVALID_ESTID;
	}
	else
	{
		auto& buy_order = get_order(_order_data->buy_order);
		if (buy_order.est_id != INVALID_ESTID)
		{
			set_cancel_condition(buy_order.est_id, [this](const tick_info& tick)->bool {

				if (tick.time > _coming_to_close)
				{
					return true;
				}
				return false;
				});
		}
		else
		{
			_order_data->buy_order = INVALID_ESTID;
		}
		auto& sell_order = get_order(_order_data->sell_order);
		if (sell_order.est_id != INVALID_ESTID)
		{
			set_cancel_condition(sell_order.est_id, [this](const tick_info& tick)->bool {

				if (tick.time > _coming_to_close)
				{
					return true;
				}
				return false;
				});
		}
		else
		{
			_order_data->sell_order = INVALID_ESTID;
		}
	}

}

void marketing_strategy::on_tick(const tick_info& tick, const deal_info& deal)
{
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
	if (_order_data->buy_order != INVALID_ESTID || _order_data->sell_order != INVALID_ESTID)
	{
		LOG_DEBUG("_buy_order or _sell_order not null  %s %llu %llu\n", tick.id.get_id(), _order_data->buy_order, _order_data->sell_order);
		return;
	}
	
	double_t sell_price = tick.sell_price() + _open_delta + _random(_random_engine);
	double_t buy_price = tick.buy_price() - _open_delta - _random(_random_engine);
	const auto& pos = get_position(_code);
	//¶àÍ·
	if (pos.yestoday_short.usable() > 0)
	{
		uint32_t buy_once = std::min(pos.yestoday_short.usable(), _open_once);
		_order_data->buy_order = buy_for_close(tick.id, buy_once, buy_price);
	}
	else if (pos.today_short.usable() > 0)
	{
		uint32_t buy_once = std::min(pos.today_short.usable(), _open_once);
		_order_data->buy_order = buy_for_close(tick.id, buy_once, buy_price);
	}
	else
	{
		_order_data->buy_order = buy_for_open(tick.id, _open_once, buy_price);
	}
	//¿ÕÍ·
	if (pos.yestoday_long.usable() > 0)
	{
		uint32_t sell_once = std::min(pos.yestoday_long.usable(), _open_once);
		_order_data->sell_order = sell_for_close(tick.id, sell_once, sell_price);
	}
	else if (pos.today_long.usable() > 0)
	{
		uint32_t sell_once = std::min(pos.today_long.usable(), _open_once);
		_order_data->sell_order = sell_for_close(tick.id, sell_once, sell_price);
	}
	else
	{
		_order_data->sell_order = sell_for_open(tick.id, _open_once, sell_price);
	}
}



void marketing_strategy::on_entrust(const order_info& order)
{
	LOG_INFO("on_entrust : %llu %s %d %d %f %d/%d\n", order.est_id, order.code, order.direction, order.offset, order.price, order.last_volume, order.total_volume);

	if (order.est_id == _order_data->buy_order || order.est_id == _order_data->sell_order)
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

void marketing_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	LOG_INFO("on_trade : %llu %s %d %d %f %d\n", localid, code, direction, offset, price, volume);
	if (localid == _order_data->buy_order)
	{
		cancel_order(_order_data->sell_order);
		_order_data->buy_order = INVALID_ESTID;
	}
	if (localid == _order_data->sell_order)
	{
		cancel_order(_order_data->buy_order);
		_order_data->sell_order = INVALID_ESTID;
	}
}

void marketing_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	LOG_INFO("on_cancel : %llu %s %d %d %f %d\n", localid, code, direction, offset, price, cancel_volume);

	if (localid == _order_data->buy_order)
	{
		_order_data->buy_order = INVALID_ESTID;
	}
	if (localid == _order_data->sell_order)
	{
		_order_data->sell_order = INVALID_ESTID;
	}
}

void marketing_strategy::on_error(error_type type, estid_t localid, const uint32_t error)
{
	LOG_ERROR("on_error : %llu %d \n", localid, error);
	if (type == error_type::ET_PLACE_ORDER)
	{
		if (localid == _order_data->buy_order)
		{
			_order_data->buy_order = INVALID_ESTID;
		}
		if (localid == _order_data->sell_order)
		{
			_order_data->sell_order = INVALID_ESTID;
		}
	}
	
}

void marketing_strategy::on_destory(lt::unsubscriber& unsuber)
{
	unsuber.unregist_tick_receiver(_code, this);
}