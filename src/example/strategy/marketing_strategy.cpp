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
#include "marketing_strategy.h"
#include "time_utils.hpp"
#include <string_helper.hpp>

using namespace lt;
using namespace lt::hft;

// 此策略作为动态链接策略需要提供DLL入口函数
strategy* create_strategy(straid_t id, syringe* syringe, const params& p)
{
	const code_t& code = p.get<code_t>("code");
	double_t open_detla = p.get<double_t>("open_detla");
	uint32_t open_once = p.get<uint32_t>("open_once");
	PRINT_INFO("create_strategy : ", id, code.to_string(), open_detla, open_once);
	return new marketing_strategy(id, syringe, code, open_detla, open_once);
}
// 此策略作为动态链接策略需要提供DLL销毁接口
void delete_strategy(lt::hft::strategy* strategy)
{
	PRINT_INFO("delete_strategy : ", strategy->get_id());
	delete strategy;
}

void marketing_strategy::on_init(subscriber& suber)
{
	suber.regist_tick_receiver(_code,this);	
}

void marketing_strategy::on_tick(const tick_info& tick)
{

	if (is_close_coming())
	{
		PRINT_DEBUG("time > _coming_to_close", tick.id.get_symbol(), tick.time);
		return;
	}
	const auto& pos = get_position(_code);
	//PRINT_INFO("on_tick time : %d.%d %s %f %llu %llu\n", tick.time,tick.tick,tick.id.get_id(), tick.price, _buy_order, _sell_order);
	if (_order_data.buy_order == INVALID_ESTID)
	{
		double_t buy_price = tick.buy_price() - _open_delta - _random(_random_engine);

		//多头
		
		if (pos.total_short.usable() > 0)
		{
			if (pos.history_short.usable() > 0)
			{
				uint32_t buy_once = std::min<uint32_t>(pos.history_short.usable(), _open_once);
				_order_data.buy_order = buy_close(tick.id, buy_once, buy_price);
			}
			else
			{
				uint32_t buy_once = std::min<uint32_t>(pos.total_short.usable(), _open_once);
				_order_data.buy_order = buy_close(tick.id, buy_once, buy_price, true);
			}
		}
		else
		{
			_order_data.buy_order = buy_open(tick.id, _open_once, buy_price);
		}
	}
	if (_order_data.sell_order == INVALID_ESTID)
	{
		double_t sell_price = tick.sell_price() + _open_delta + _random(_random_engine);

		//空头
		if (pos.history_long.usable() > 0)
		{
			uint32_t sell_once = std::min<uint32_t>(pos.history_long.usable(), _open_once);
			_order_data.sell_order = sell_close(tick.id, sell_once, sell_price);
		}
		else if (pos.total_long.usable() > 0)
		{
			uint32_t sell_once = std::min<uint32_t>(pos.total_long.usable(), _open_once);
			_order_data.sell_order = sell_close(tick.id, sell_once, sell_price, true);
		}
		else
		{
			_order_data.sell_order = sell_open(tick.id, _open_once, sell_price);
		}
	}
}



void marketing_strategy::on_entrust(const order_info& order)
{
	PRINT_INFO("on_entrust :", order.estid, order.code.get_symbol(), order.direction, order.offset, order.price, order.last_volume, order.total_volume);

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

void marketing_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	PRINT_INFO("on_trade :", localid, code.get_symbol(), direction, offset, price, volume);
	if (localid == _order_data.buy_order)
	{
		cancel_order(_order_data.sell_order);
		_order_data.buy_order = INVALID_ESTID;
	}
	if (localid == _order_data.sell_order)
	{
		cancel_order(_order_data.buy_order);
		_order_data.sell_order = INVALID_ESTID;
	}
}

void marketing_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	PRINT_INFO("on_cancel :", localid, code.get_symbol(), direction, offset, price, cancel_volume);

	if (localid == _order_data.buy_order)
	{
		_order_data.buy_order = INVALID_ESTID;
	}
	if (localid == _order_data.sell_order)
	{
		_order_data.sell_order = INVALID_ESTID;
	}
}

void marketing_strategy::on_error(error_type type, estid_t localid, const error_code error)
{
	PRINT_ERROR("on_error :", localid, error);
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

void marketing_strategy::on_destroy(unsubscriber& unsuber)
{
	unsuber.unregist_tick_receiver(_code, this);
}

bool marketing_strategy::is_close_coming()const {
	return make_daytm("15:13:00", 0U) < get_last_time();
}