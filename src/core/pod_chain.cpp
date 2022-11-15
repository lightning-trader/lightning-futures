#include "pod_chain.h"
#include <trader_api.h>

uint32_t pod_chain::get_pending_position(code_t code) const
{
	std::vector<const order_info*> order_list;
	_trader->find_orders(order_list, [code](const order_info& order)->bool {
		return order.code == code;
		});
	uint32_t res = 0;
	for (auto& it : order_list)
	{
		res += it->last_volume;
	}
	return res;
}

estid_t close_to_open_chain::place_order(offset_type offset, direction_type direction, code_t code, uint32_t count, double_t price, order_flag flag)
{
	if (offset == OT_CLOSE)
	{
		uint32_t total_position = 0;
		auto position = _trader->get_position(code);
		if(position != nullptr)
		{
			total_position = position->get_total() + get_pending_position(code);
		}
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

estid_t open_to_close_chain::place_order(offset_type offset, direction_type direction, code_t code, uint32_t count, double_t price, order_flag flag)
{
	//暂时先不处理拆单情况了，因为拆弹会导致上层接口变得复杂，所以只有持有单量大于等于需要单量才起作用
	if(offset == OT_OPEN)
	{
		if(direction == DT_LONG)
		{
			auto position = _trader->get_position(code);
			if (position && position->short_postion >= count)
			{
				//开多转平空
				return _next->place_order(OT_CLOSE, DT_SHORT, code, count, price, flag);
			}
		}
		if (direction == DT_SHORT)
		{
			auto position = _trader->get_position(code);
			if (position && position->long_postion >= count)
			{
				//开空转平多
				return _next->place_order(OT_CLOSE, DT_LONG, code, count, price, flag);
			}
		}
	}
	return _next->place_order(offset, direction, code, count, price, flag);
}


estid_t price_to_cancel_chain::place_order(offset_type offset, direction_type direction, code_t code, uint32_t count, double_t price, order_flag flag)
{
	std::vector<const order_info*> order_list ;
	if(direction == DT_LONG)
	{
		_trader->find_orders(order_list,[](const order_info& order)->bool{
			return order.direction == DT_SHORT;
		});
	}
	if (direction == DT_SHORT)
	{
		_trader->find_orders(order_list, [](const order_info& order)->bool {
			return order.direction == DT_LONG;
			});
	}
	const order_info* entrust = nullptr;
	for (const auto& it : order_list)
	{
		if (it->last_volume == count)
		{
			entrust = it;
			break;
		}
	}
	if (entrust != nullptr)
	{
		_trader->cancel_order(entrust->est_id);
		return INVALID_ESTID;
	}
	return _next->place_order(offset, direction, code, count, price, flag);
}


estid_t the_end_chain::place_order(offset_type offset, direction_type direction, code_t code, uint32_t count, double_t price, order_flag flag)
{
	if(offset == OT_OPEN)
	{
		auto position = _trader->get_position(code);
		auto pending = get_pending_position(code);
		if (position && position->get_total()+ pending + count > _max_position)
		{
			return INVALID_ESTID;
		}
	}
	else if (offset == OT_CLOSE)
	{
		const auto pos = _trader->get_position(code);
		if (pos && direction == DT_LONG && pos->long_postion - pos->long_frozen < count)
		{
			return INVALID_ESTID;
		}
		else if (pos && direction == DT_SHORT && pos->short_postion - pos->short_frozen < count)
		{
			return INVALID_ESTID;
		}
	}
	return _trader->place_order(offset, direction, code, count, price, flag);
}

