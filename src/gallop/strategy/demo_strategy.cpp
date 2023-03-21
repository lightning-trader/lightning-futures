#include "demo_strategy.h"
#include "time_utils.hpp"

demo_strategy::demo_strategy(const param& p) :
	strategy(),
	_order_data(nullptr),
	_coming_to_close(0),
	_random(0, p.get<uint32_t>("random_offset"))
{
	_code = p.get<const char*>("code");
	_open_once = p.get<uint32_t>("open_once");
	_open_delta = p.get<double_t>("open_delta");
	_yestoday_multiple = p.get<uint32_t>("yestoday_multiple");
	_yestoday_threshold = p.get<uint32_t>("yestoday_threshold");
	_yestoday_growth = p.get<double_t>("yestoday_growth");
	_expire = p.get<const char*>("expire");
};

void demo_strategy::on_init()
{
	subscribe(_code);
	if (_expire != default_code)
	{
		subscribe(_expire);
	}
	use_custom_chain(TO_OPEN_TO_CLOSE, false);
	_order_data = static_cast<persist_data*>(get_userdata(sizeof(persist_data)));
}

void demo_strategy::on_ready()
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

void demo_strategy::on_tick(const tick_info& tick, const deal_info& deal)
{
	_last_tick = tick;
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
	double_t delta = (tick.standard * _open_delta);
	double_t sell_price = tick.sell_price() + delta + _random(_random_engine);
	uint32_t sell_once = _open_once;
	uint32_t yestoday_once = _yestoday_multiple * _open_once;
	double_t buy_price = tick.buy_price() - delta - _random(_random_engine);
	uint32_t buy_once = _open_once;
	if (tick.id == _expire)
	{
		//对于过期合约，只平仓不开仓
		const position_info& expire_pos = get_position(_expire);

		if (expire_pos.yestoday_long.usable() > 0)
		{
			if (expire_pos.yestoday_long.usable() > _yestoday_threshold)
			{
				yestoday_once = static_cast<uint32_t>(yestoday_once * _yestoday_growth);
			}
			sell_once = expire_pos.yestoday_long.usable() > yestoday_once ? yestoday_once : expire_pos.yestoday_long.usable();
			sell_price += (sell_once / _open_once) * delta / 2.F;
			sell_price = std::round(sell_price);
			if (sell_price < tick.high_limit)
			{
				_order_data->sell_order = sell_for_open(tick.id, sell_once, sell_price);
			}
		}

		if (expire_pos.yestoday_short.usable() > 0)
		{
			if (expire_pos.yestoday_short.usable() > _yestoday_threshold)
			{
				yestoday_once = static_cast<uint32_t>(yestoday_once * _yestoday_growth);
			}
			buy_once = expire_pos.yestoday_short.usable() > yestoday_once ? yestoday_once : expire_pos.yestoday_short.usable();
			buy_price -= (buy_once / _open_once) * delta / 2.F;
			buy_price = std::round(buy_price);
			if (buy_price > tick.low_limit)
			{
				_order_data->buy_order = buy_for_open(tick.id, buy_once, buy_price);
			}
		}
	}
	else
	{

		//对于普通合约，过期合约存在的情况下，不平仓只开仓
		const position_info& pos = get_position(_code);
		const position_info& expire_pos = get_position(_expire);
		if (expire_pos.yestoday_long.usable() == 0)
		{
			if (pos.yestoday_long.usable() > 0)
			{
				if (pos.yestoday_long.usable() > _yestoday_threshold)
				{
					yestoday_once = static_cast<uint32_t>(yestoday_once * _yestoday_growth);
				}
				sell_once = pos.yestoday_long.usable() > yestoday_once ? yestoday_once : pos.yestoday_long.usable();
				sell_price += (sell_once / _open_once) * delta / 2.F;
			}
			sell_price = std::round(sell_price);
			if (sell_price < tick.high_limit)
			{
				_order_data->sell_order = sell_for_open(tick.id, sell_once, sell_price);
			}
		}
		if (expire_pos.yestoday_short.usable() == 0)
		{
			if (pos.yestoday_short.usable() > 0)
			{
				if (pos.yestoday_short.usable() > _yestoday_threshold)
				{
					yestoday_once = static_cast<uint32_t>(yestoday_once * _yestoday_growth);
				}
				buy_once = pos.yestoday_short.usable() > yestoday_once ? yestoday_once : pos.yestoday_short.usable();
				buy_price -= (buy_once / _open_once) * delta / 2.F;
			}
			buy_price = std::round(buy_price);
			if (buy_price > tick.low_limit)
			{
				_order_data->buy_order = buy_for_open(tick.id, buy_once, buy_price);
			}
		}
	}
}



void demo_strategy::on_entrust(const order_info& order)
{
	LOG_INFO("emg_1_strategy on_entrust : %llu %s %d %d %f %d/%d\n", order.est_id, order.code, order.direction, order.offset, order.price, order.last_volume, order.total_volume);
	if (_last_tick.time > _coming_to_close)
	{
		return;
	}
	if (order.est_id == _order_data->buy_order || order.est_id == _order_data->sell_order)
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

void demo_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	LOG_INFO("emg_1_strategy on_trade : %llu %s %d %d %f %d\n", localid, code, direction, offset, price, volume);
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

void demo_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	LOG_INFO("emg_1_strategy on_cancel : %llu %s %d %d %f %d\n", localid, code, direction, offset, price, cancel_volume);

	if (localid == _order_data->buy_order)
	{
		_order_data->buy_order = INVALID_ESTID;
	}
	if (localid == _order_data->sell_order)
	{
		_order_data->sell_order = INVALID_ESTID;
	}
}

void demo_strategy::on_error(error_type type, estid_t localid, const uint32_t error)
{
	LOG_ERROR("emg_1_strategy on_error : %llu %d \n", localid, error);
	if (type != ET_PLACE_ORDER)
	{
		return;
	}
	if (localid == _order_data->buy_order)
	{
		_order_data->buy_order = INVALID_ESTID;
	}
	if (localid == _order_data->sell_order)
	{
		_order_data->sell_order = INVALID_ESTID;
	}
}

void demo_strategy::on_destory()
{
	unsubscribe(_code);
	if (_expire != default_code)
	{
		unsubscribe(_expire);
	}
}