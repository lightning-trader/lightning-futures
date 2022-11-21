#include "order_container.h"


void order_container::add_order(const order_info& order)
{
	spin_lock lock(_mutex);
	_order_info[order.est_id] = order;
	LOG_TRACE("order_container add_order  %lld %d ", order.est_id, _order_info.size());
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
		LOG_TRACE("order_container del_order %lld ", estid);
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

uint32_t order_container::set_last_volume(estid_t estid, uint32_t last_volume)
{
	spin_lock lock(_mutex);
	auto it = _order_info.find(estid);
	if (it != _order_info.end())
	{
		it->second.last_volume = last_volume;
		return it->second.last_volume;
	}
	return 0;
}

double_t order_container::set_price(estid_t estid, double_t price)
{
	spin_lock lock(_mutex);
	auto it = _order_info.find(estid);
	if(it != _order_info.end())
	{
		it->second.price = price;
	}
	return it->second.price;
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
	LOG_TRACE("order_container get_order_info  %lld %d ", estid, _order_info.size());
	auto& odit = _order_info.find(estid);
	if (odit != _order_info.end())
	{
		order = odit->second;
		LOG_TRACE("order_container get_order_info order %lld %d ", order.est_id, _order_info.size());
		return true;
	}
	LOG_TRACE("order_container get_order_info false");
	return false;
}

const std::map<estid_t, order_info> order_container::get_all_order()const
{
	spin_lock lock(_mutex);
	return _order_info;
}