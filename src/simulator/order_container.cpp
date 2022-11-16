#include "order_container.h"
#include <log_wapper.hpp>


void order_container::add_order(const order_info& order)
{
	spin_lock lock(_mutex);
	_order_info[order.est_id] = order;
	_order_match[order.code].emplace_back(order.est_id);
}

void order_container::del_order(estid_t estid)
{
	spin_lock lock(_mutex);
	auto& odit = _order_info.find(estid);
	if (odit != _order_info.end())
	{
		auto match = _order_match.find(odit->second.code);
		if (match != _order_match.end())
		{
			auto mch_odr = std::find_if(match->second.begin(), match->second.end(), [estid](const order_match& p)->bool {
				return p.est_id == estid;
				});
			if (mch_odr != match->second.end())
			{
				match->second.erase(mch_odr);
			}
		}
		_order_info.erase(odit);
	}
}

void order_container::set_seat(estid_t estid, uint32_t seat)
{
	spin_lock lock(_mutex);
	auto& odit = _order_info.find(estid);
	if (odit != _order_info.end())
	{
		auto it = _order_match.find(odit->second.code);
		if (it != _order_match.end())
		{
			auto od_it = std::find_if(it->second.begin(), it->second.end(), [estid](const order_match& cur) ->bool {

				return cur.est_id == estid;
				});
			if (od_it != it->second.end())
			{
				od_it->queue_seat = seat;
			}
		}
	}
}

void order_container::set_state(estid_t estid, order_state state)
{
	spin_lock lock(_mutex);
	auto& odit = _order_info.find(estid);
	if (odit != _order_info.end())
	{
		auto it = _order_match.find(odit->second.code);
		if (it != _order_match.end())
		{
			auto od_it = std::find_if(it->second.begin(), it->second.end(), [estid](const order_match& cur) ->bool {

				return cur.est_id == estid;
				});
			if (od_it != it->second.end())
			{
				od_it->state = state;
			}
		}
	}
	
}

void order_container::set_last_volume(estid_t estid, uint32_t last_volume)
{
	spin_lock lock(_mutex);
	auto it = _order_info.find(estid);
	if (it != _order_info.end())
	{
		it->second.last_volume = last_volume;
	}
}

void order_container::set_price(estid_t estid, double_t price)
{
	spin_lock lock(_mutex);
	auto it = _order_info.find(estid);
	if(it != _order_info.end())
	{
		it->second.price = price;
	}
}

void order_container::get_order_match(std::vector<order_match>& match_list, const code_t& code)const
{
	spin_lock lock(_mutex);
	auto it = _order_match.find(code);
	if (it != _order_match.end())
	{
		for (auto& od_it : it->second)
		{
			match_list.emplace_back(od_it);
		}
	}
}

bool order_container::get_order_info(order_info& order, estid_t estid)const
{
	spin_lock lock(_mutex);
	auto& odit = _order_info.find(estid);
	if (odit != _order_info.end())
	{
		order = odit->second;
		return true;
	}
	return false;
}

const std::map<estid_t, order_info> order_container::get_all_order()const
{
	spin_lock lock(_mutex);
	return _order_info;
}