/*
Distributed under the MIT License(MIT)

Copyright(c) 2023 Jihua Zou EMail: ghuazo@qq.com QQ:137336521

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in the
Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "orderflow_strategy.h"
#include "time_utils.hpp"
#include <string_helper.hpp>

using namespace lt;
using namespace lt::hft;


void orderflow_strategy::on_init(subscriber& suber)
{
	suber.regist_bar_receiver(_code, _period,this);
}

void orderflow_strategy::on_bar(const lt::bar_info& bar)
{

	auto unbalance = bar.get_unbalance(_multiple);
	if(_order_data.buy_order == INVALID_ESTID)
	{
		//可以买入的
		//需求失衡，说明有买方力量大于卖方力量，顺势而为则买入
		auto ub = unbalance.first;
		if(ub->size() > _threshold)
		{
			try_buy();
		}
	}
	if (_order_data.sell_order == INVALID_ESTID)
	{
		//可以卖出的
		//需求失衡，说明有买方力量大于买方力量，顺势而为则卖出
		auto ub = unbalance.second;
		if (ub->size() > _threshold)
		{
			try_sell();
		}
	}
}


void orderflow_strategy::on_entrust(const order_info& order)
{
	LOG_INFO("on_entrust :", order.estid, order.code.get_symbol(), order.direction, order.offset, order.price, order.last_volume, order.total_volume);

	if (order.estid == _order_data.buy_order || order.estid == _order_data.sell_order)
	{
		set_cancel_condition(order.estid, [this](estid_t estid)->bool {

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
	LOG_INFO("on_trade :", localid, code.get_symbol(), direction, offset, price, volume);
	if (localid == _order_data.buy_order)
	{
		_order_data.buy_order = INVALID_ESTID;
	}
	if (localid == _order_data.sell_order)
	{
		_order_data.sell_order = INVALID_ESTID;
	}
}

void orderflow_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	LOG_INFO("on_cancel :", localid, code.get_symbol(), direction, offset, price, cancel_volume);

	if (localid == _order_data.buy_order)
	{
		_order_data.buy_order = INVALID_ESTID;
	}
	if (localid == _order_data.sell_order)
	{
		_order_data.sell_order = INVALID_ESTID;
	}
}

void orderflow_strategy::on_error(error_type type, estid_t localid, const error_code error)
{
	LOG_ERROR("on_error :", localid, error);
	if (type == error_type::ET_PLACE_ORDER)
	{
		if (localid == _order_data.buy_order)
		{
			_order_data.buy_order = INVALID_ESTID;
		}
		if (localid == _order_data.sell_order)
		{
			_order_data.sell_order = INVALID_ESTID;
		}
	}
	
}

void orderflow_strategy::on_destroy(unsubscriber& unsuber)
{
	unsuber.unregist_bar_receiver(_code, _period, this);
}

void orderflow_strategy::try_buy()
{
	const auto& market = get_market_info(_code);
	const auto& pos = get_position(_code);
	if(pos.history_short.usable()>0)
	{
		auto volume = std::min<uint32_t>(_open_once, pos.history_short.usable());
		_order_data.buy_order = buy_close(_code, volume, market.last_tick_info.sell_price());
		return ;
	}
	if (pos.today_short.usable() > 0)
	{
		auto volume = std::min<uint32_t>(_open_once, pos.today_short.usable());
		_order_data.buy_order = buy_close(_code, volume, market.last_tick_info.sell_price(), true);
		return;
	}
	if(_open_once + pos.get_long_position() + pos.long_pending < _position_limit)
	{
		_order_data.buy_order = buy_open(_code, _open_once, market.last_tick_info.sell_price());
	}
}

void orderflow_strategy::try_sell()
{
	const auto& market = get_market_info(_code);
	const auto& pos = get_position(_code);
	if (pos.history_long.usable() > 0)
	{
		auto min = std::min<uint32_t>(_open_once, pos.history_long.usable());
		_order_data.sell_order = sell_close(_code, min, market.last_tick_info.buy_price());
		return;
	}
	if (pos.today_long.usable() > 0)
	{
		auto min = std::min<uint32_t>(_open_once, pos.today_long.usable());
		_order_data.sell_order = sell_close(_code, min, market.last_tick_info.buy_price(), true);
		return;
	}
	if (_open_once + pos.get_short_position() + pos.short_pending < _position_limit)
	{
		_order_data.sell_order = sell_open(_code, _open_once, market.last_tick_info.buy_price());
	}
}


bool orderflow_strategy::is_close_coming()const {
	return make_daytm("14:58:00", 0U) < get_last_time();
}