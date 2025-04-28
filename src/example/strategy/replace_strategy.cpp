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

#include "replace_strategy.h"
#include "time_utils.hpp"
#include <string_helper.hpp>

using namespace lt;
using namespace lt::hft;


void replace_strategy::on_init(subscriber& suber)
{
	suber.regist_tape_receiver(_code, this);
}

void replace_strategy::on_tape(const lt::tape_info& tape)
{
	LOG_INFO(tape.price,static_cast<int32_t>(tape.direction))
}

void replace_strategy::on_change(const lt::params& p)
{
	int8_t ratio = p.get<int8_t>("ratio");
	if(ratio == 1)
	{
		try_buy();
	}
	else if(ratio == -1)
	{
		try_sell();
	}
	else
	{
		try_close();
	}
}

void replace_strategy::on_entrust(const order_info& order)
{
	LOG_INFO("on_entrust :", order.estid, order.code.get_id(), order.direction, order.offset, order.price, order.last_volume, order.total_volume);
}

void replace_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	LOG_INFO("on_trade :", localid, code.get_id(), direction, offset, price, volume);
	for (size_t i = 0; i < ORDER_ESTID_COUNT; i++)
	{
		if (order_estids[i] != INVALID_ESTID)
		{
			order_estids[i] = INVALID_ESTID ;
		}
	}
}

void replace_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	LOG_INFO("on_cancel :", localid, code.get_id(), direction, offset, price, cancel_volume);

	for (size_t i = 0; i < ORDER_ESTID_COUNT; i++)
	{
		if (order_estids[i] != INVALID_ESTID)
		{
			order_estids[i] = INVALID_ESTID;
		}
	}
}

void replace_strategy::on_error(error_type type, estid_t localid, const error_code error)
{
	LOG_ERROR("on_error :", localid, error);
	for (size_t i = 0; i < ORDER_ESTID_COUNT; i++)
	{
		if (order_estids[i] != INVALID_ESTID)
		{
			order_estids[i] = INVALID_ESTID;
		}
	}
	
}

void replace_strategy::on_destroy(unsubscriber& unsuber)
{
	unsuber.unregist_tape_receiver(_code, this);
}

void replace_strategy::try_buy()
{
	const auto& market = get_market_info(_code);
	const auto& pos = get_position(_code);
	if(pos.history_short.usable()>0)
	{
		order_estids[SHORT_ClOSE_HISTORY] = buy_close(_code, pos.history_short.usable(), market.last_tick_info.sell_price());
	}
	if (pos.today_short.usable() > 0)
	{
		order_estids[SHORT_ClOSE_TODAY] = buy_close(_code, pos.today_short.usable(), market.last_tick_info.sell_price(), true);
	}
	order_estids[LONG_OPEN_TODAY] = buy_open(_code, _open_once, market.last_tick_info.sell_price());

}

void replace_strategy::try_sell()
{
	const auto& market = get_market_info(_code);
	const auto& pos = get_position(_code);
	if (pos.history_long.usable() > 0)
	{
		order_estids[LONG_ClOSE_HISTORY] = sell_close(_code, pos.history_long.usable(), market.last_tick_info.buy_price());
	}
	if (pos.today_long.usable() > 0)
	{
		order_estids[LONG_ClOSE_TODAY] = sell_close(_code, pos.today_long.usable(), market.last_tick_info.buy_price(), true);
	}
	order_estids[SHORT_OPEN_TODAY] = sell_open(_code, _open_once, market.last_tick_info.buy_price());
}


void replace_strategy::try_close()
{
	const auto& market = get_market_info(_code);
	const auto& pos = get_position(_code);
	if (pos.history_long.usable() > 0)
	{
		order_estids[LONG_ClOSE_HISTORY] = sell_close(_code, pos.history_long.usable(), market.last_tick_info.buy_price());
	}
	if (pos.today_long.usable() > 0)
	{
		order_estids[LONG_ClOSE_TODAY] = sell_close(_code, pos.today_long.usable(), market.last_tick_info.buy_price(), true);
	}
	if (pos.history_short.usable() > 0)
	{
		order_estids[SHORT_ClOSE_HISTORY] = buy_close(_code, pos.history_short.usable(), market.last_tick_info.sell_price());
	}
	if (pos.today_short.usable() > 0)
	{
		order_estids[SHORT_ClOSE_TODAY] = buy_close(_code, pos.today_short.usable(), market.last_tick_info.sell_price(), true);
	}
}