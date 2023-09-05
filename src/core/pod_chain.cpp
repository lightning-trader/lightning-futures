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
		_trader.cancel_order(order_list.begin()->est_id);
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
			return INVALID_ESTID;
		}
	}
	else if (offset == offset_type::OT_CLOSE)
	{
		const auto pos = _ctx.get_position(code);
		if (direction == direction_type::DT_LONG &&pos.history_long.usable() < count)
		{
			return INVALID_ESTID;
		}
		else if (direction == direction_type::DT_SHORT && pos.history_short.usable() < count)
		{
			return INVALID_ESTID;
		}
	}
	else if (offset == offset_type::OT_CLSTD)
	{
		const auto pos = _ctx.get_position(code);
		if (direction == direction_type::DT_LONG && pos.today_long.usable() < count )
		{
			return INVALID_ESTID;
		}
		else if (direction == direction_type::DT_SHORT && pos.today_short.usable() < count )
		{
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

