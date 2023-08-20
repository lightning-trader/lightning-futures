#include "orderflow_strategy.h"
#include "time_utils.hpp"
#include <string_helper.hpp>
#include <mmf_wapper.hpp>

using namespace lt;


void orderflow_strategy::on_init(subscriber& suber)
{
	suber.regist_bar_receiver(_code, _period,this);
	use_custom_chain(true);
	_order_data = maping_file<persist_data>(string_helper::format("./orderflow_strategy_{0}.mmf", get_id()).c_str());
}

void orderflow_strategy::on_ready()
{
	uint32_t trading_day = get_trading_day();
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
			set_cancel_condition(buy_order.est_id, [this]()->bool {

				if (is_close_coming())
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
			set_cancel_condition(sell_order.est_id, [this]()->bool {

				if (is_close_coming())
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

void orderflow_strategy::on_bar(uint32_t period, const bar_info& bar)
{
	if(_order_data->buy_order == INVALID_ESTID)
	{
		//可以买入的
		auto ub = bar.buy_unbalance(_delta);
		if(ub.size() > _limit)
		{
			try_buy();
		}
	}
	if (_order_data->sell_order == INVALID_ESTID)
	{
		//可以卖出的
		auto ub = bar.sell_unbalance(_delta) ;
		if (ub.size() > _limit)
		{
			try_sell();
		}
	}
}


void orderflow_strategy::on_entrust(const order_info& order)
{
	LOG_INFO("on_entrust :", order.est_id, order.code.get_id(), order.direction, order.offset, order.price, order.last_volume, order.total_volume);

	if (order.est_id == _order_data->buy_order || order.est_id == _order_data->sell_order)
	{
		set_cancel_condition(order.est_id, [this]()->bool {

			if (is_close_coming())
			{
				return true;
			}
			return false;
			});
	}
}

void orderflow_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	LOG_INFO("on_trade :", localid, code.get_id(), direction, offset, price, volume);
	if (localid == _order_data->buy_order)
	{
		_order_data->buy_order = INVALID_ESTID;
	}
	if (localid == _order_data->sell_order)
	{
		_order_data->sell_order = INVALID_ESTID;
	}
}

void orderflow_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	LOG_INFO("on_cancel :", localid, code.get_id(), direction, offset, price, cancel_volume);

	if (localid == _order_data->buy_order)
	{
		_order_data->buy_order = INVALID_ESTID;
	}
	if (localid == _order_data->sell_order)
	{
		_order_data->sell_order = INVALID_ESTID;
	}
}

void orderflow_strategy::on_error(error_type type, estid_t localid, const uint32_t error)
{
	LOG_ERROR("on_error :", localid, error);
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

void orderflow_strategy::on_destroy(lt::unsubscriber& unsuber)
{
	unsuber.unregist_bar_receiver(_code, _period);
	unmaping_file(_order_data);
}

void orderflow_strategy::try_buy()
{
	auto& tick = get_last_tick(_code);
	auto pos = get_position(_code);
	if(pos.yestoday_short.usable()>0)
	{
		auto min = std::min(_open_once, pos.yestoday_short.usable());
		_order_data->buy_order = buy_for_close(_code, min, tick.sell_price());
		return ;
	}
	if (pos.today_short.usable() > 0)
	{
		auto min = std::min(_open_once, pos.today_short.usable());
		_order_data->buy_order = buy_for_close(_code, min, tick.sell_price());
		return;
	}
	_order_data->buy_order = buy_for_open(_code, _open_once, tick.sell_price());
}

void orderflow_strategy::try_sell()
{
	auto& tick = get_last_tick(_code);
	auto pos = get_position(_code);
	if (pos.yestoday_long.usable() > 0)
	{
		auto min = std::min(_open_once, pos.yestoday_long.usable());
		_order_data->sell_order = sell_for_close(_code, min, tick.buy_price());
		return;
	}
	if (pos.today_long.usable() > 0)
	{
		auto min = std::min(_open_once, pos.today_long.usable());
		_order_data->sell_order = sell_for_close(_code, min, tick.buy_price());
		return;
	}
	_order_data->sell_order = sell_for_close(_code, _open_once, tick.buy_price());
}