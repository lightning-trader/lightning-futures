#include "pod_chain.h"
#include <trader_api.h>

uint32_t pod_chain::get_open_pending() const
{
	std::vector<order_info> order_list;
	_trader->find_orders(order_list, [](const order_info& order)->bool {
		return order.offset==OT_OPEN;
		});
	uint32_t res = 0;
	for (auto& it : order_list)
	{
		res += it.last_volume;
	}
	return res;
}

estid_t close_to_open_chain::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	LOG_DEBUG("close_to_open_chain place_order %s", code.get_id());
	if (offset == OT_CLOSE)
	{
		uint32_t total_position = 0;
		auto position = _trader->get_total_position();
		total_position = position + get_open_pending();
		
		if (direction == DT_LONG)
		{
			if (total_position + count <= _max_position)
			{
				//平多转开空
				return _next->place_order(OT_OPEN, DT_SHORT, code, count, price, flag);
			}
		}
		if (direction == DT_SHORT)
		{
			if (total_position + count <= _max_position)
			{
				//平空转开多
				return _next->place_order(OT_OPEN, DT_LONG, code, count, price, flag);
			}
		}

	}

	return _next->place_order(offset, direction, code, count, price, flag);
}

estid_t open_to_close_chain::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	LOG_DEBUG("open_to_close_chain place_order %s", code.get_id());
	//暂时先不处理拆单情况了，因为拆弹会导致上层接口变得复杂，所以只有持有单量大于等于需要单量才起作用
	if(offset == OT_OPEN)
	{
		if(direction == DT_LONG)
		{
			auto pos = _trader->get_position(code);
			LOG_INFO("open_to_close_chain place_order DT_LONG %s %d %d %d", code.get_id(), pos.today_short.usable(), pos.yestoday_short.usable(), count);
			if (pos.today_short.usable() >= count || pos.yestoday_short.usable() >= count)
			{
				//开多转平空
				return _next->place_order(OT_CLOSE, DT_SHORT, code, count, price, flag);
			}
		}
		if (direction == DT_SHORT)
		{
			auto pos = _trader->get_position(code);
			LOG_INFO("open_to_close_chain place_order DT_SHORT %s %d %d %d", code.get_id(), pos.today_long.usable(), pos.yestoday_long.usable(), count);
			if (pos.today_long.usable() >= count || pos.yestoday_long.usable() >= count)
			{
				//开空转平多
				return _next->place_order(OT_CLOSE, DT_LONG, code, count, price, flag);
			}
		}
	}
	return _next->place_order(offset, direction, code, count, price, flag);
}


estid_t price_to_cancel_chain::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	LOG_DEBUG("price_to_cancel_chain place_order %s", code.get_id());
	std::vector<order_info> order_list ;
	if(direction == DT_LONG)
	{
		_trader->find_orders(order_list,[code,count, price](const order_info& order)->bool{
			return order.direction == DT_SHORT&&order.code == code && order.last_volume == count && order.price == price;
		});
	}
	if (direction == DT_SHORT)
	{
		_trader->find_orders(order_list, [code,count, price](const order_info& order)->bool {
			return order.direction == DT_LONG && order.code == code && order.last_volume == count && order.price == price;
			});
	}
	if (!order_list.empty())
	{
		_trader->cancel_order(order_list.begin()->est_id);
		return INVALID_ESTID;
	}
	return _next->place_order(offset, direction, code, count, price, flag);
}


estid_t verify_chain::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	LOG_DEBUG("verify_chain place_order %s", code.get_id());
	if(offset == OT_OPEN)
	{
		auto position = _trader->get_total_position();
		auto pending = get_open_pending();
		if (position + pending + count > _max_position)
		{
			return INVALID_ESTID;
		}
	}
	else if (offset == OT_CLOSE)
	{
		const auto pos = _trader->get_position(code);
		if (direction == DT_LONG &&( pos.today_long.usable() < count && pos.yestoday_long.usable() < count))
		{
			return INVALID_ESTID;
		}
		else if (direction == DT_SHORT && (pos.today_short.usable() < count && pos.yestoday_short.usable() < count))
		{
			return INVALID_ESTID;
		}
	}
	if(!_filter_callback(code, offset, direction, count, price, flag))
	{
		return INVALID_ESTID;
	}
	LOG_DEBUG("verify_chain _trader place_order %s", code.get_id());
	return _trader->place_order(offset, direction, code, count, price, flag);
}

