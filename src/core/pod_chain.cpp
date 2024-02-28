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
#include "pod_chain.h"
#include "context.h"
#include <trader_api.h>

pod_chain::pod_chain(context& ctx, pod_chain* next) :_next(next), _ctx(ctx), _trader(ctx.get_trader())
{
	
}
pod_chain::~pod_chain()
{

	if (_next)
	{
		delete _next;
		_next = nullptr;
	}
}


estid_t price_to_cancel_chain::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	PROFILE_DEBUG(code.get_id());
	LOG_DEBUG("price_to_cancel_chain place_order %s", code.get_id());
	std::vector<order_info> order_list ;
	if(direction == direction_type::DT_LONG)
	{
		_ctx.find_orders(order_list,[code,count, price](const order_info& order)->bool{
			return order.direction == direction_type::DT_SHORT&&order.code == code && order.last_volume == count && order.price == price;
		});
	}
	if (direction == direction_type::DT_SHORT)
	{
		_ctx.find_orders(order_list, [code,count, price](const order_info& order)->bool {
			return order.direction == direction_type::DT_LONG && order.code == code && order.last_volume == count && order.price == price;
			});
	}
	if (!order_list.empty())
	{
		_trader.cancel_order(order_list.begin()->estid);
		LOG_INFO("place order to cancel ", order_list.begin()->estid, code.get_id());
		return INVALID_ESTID;
	}
	PROFILE_DEBUG(code.get_id());
	return _next->place_order(offset, direction, code, count, price, flag);
}


estid_t verify_chain::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	PROFILE_DEBUG(code.get_id());
	LOG_DEBUG("verify_chain place_order %s", code.get_id());
	if(offset == offset_type::OT_OPEN)
	{
		auto position = _ctx.get_total_position();
		auto pending = _ctx.get_total_pending();
		auto max_position = _ctx.get_max_position();
		if (position + pending + count > max_position)
		{
			LOG_WARNING("can not open order : ", code.get_id(), position, pending, max_position);
			return INVALID_ESTID;
		}
	}
	else if (offset == offset_type::OT_CLOSE)
	{
		const auto pos = _ctx.get_position(code);
		if (direction == direction_type::DT_LONG && pos.history_long.usable() < count)
		{
			LOG_WARNING("can not close history long order : ", code.get_id(), direction, pos.history_long.usable(), count);
			return INVALID_ESTID;
		}
		else if (direction == direction_type::DT_SHORT && pos.history_short.usable() < count)
		{
			LOG_WARNING("can not close history short order : ", code.get_id(), direction, pos.history_long.usable(), count);
			return INVALID_ESTID;
		}
	}
	else if (offset == offset_type::OT_CLSTD)
	{
		const auto pos = _ctx.get_position(code);
		if (direction == direction_type::DT_LONG && pos.today_long.usable() < count )
		{
			LOG_WARNING("can not close today long order : ", code.get_id(), direction, pos.today_long.usable(), count);
			return INVALID_ESTID;
		}
		else if (direction == direction_type::DT_SHORT && pos.today_short.usable() < count )
		{
			LOG_WARNING("can not close today short order : ", code.get_id(),direction, pos.today_short.usable(), count);
			return INVALID_ESTID;
		}
	}
	auto filter_callback = _ctx.get_trading_filter();
	if(filter_callback&&!filter_callback(code, offset, direction, count, price, flag))
	{
		return INVALID_ESTID;
	}
	LOG_DEBUG("verify_chain _trader place_order %s", code.get_id());
	PROFILE_DEBUG(code.get_id());
	return _trader.place_order(offset, direction, code, count, price, flag);
}

