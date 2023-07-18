#include "order_container.h"
#include <log_wapper.hpp>

order_container::order_container()
{}

order_container::~order_container()
{
	for (auto it : _order_match)
	{
		for (auto mh : it.second)
		{
			delete mh;
		}
	}
}

void order_container::add_order(const order_info& order,order_flag flag,bool is_today)
{
	spin_lock lock(_mutex);
	_order_info[order.est_id] = order;
	LOG_TRACE("order_container add_order", order.code.get_id(), order.est_id, _order_info.size());
	auto omh = new order_match((_order_info[order.est_id]), flag);
	omh->is_today = is_today;
	_order_match[order.code].emplace_back(omh);
}

void order_container::del_order(estid_t estid)
{
	spin_lock lock(_mutex);
	auto odit = _order_info.find(estid);
	if (odit != _order_info.end())
	{
		auto match = _order_match.find(odit->second.code);
		if (match != _order_match.end())
		{
			auto mch_odr = std::find_if(match->second.begin(), match->second.end(), [estid](const order_match* p)->bool {
				return p&&p->order.est_id == estid;
				});
			if (mch_odr != match->second.end())
			{
				delete *mch_odr;
				match->second.erase(mch_odr);
			}
		}
		LOG_INFO("order_container del_order", estid);
		_order_info.erase(odit);
	}
}

void order_container::set_seat(estid_t estid, uint32_t seat)
{
	spin_lock lock(_mutex);
	auto odit = _order_info.find(estid);
	if (odit != _order_info.end())
	{
		auto it = _order_match.find(odit->second.code);
		if (it != _order_match.end())
		{
			auto od_it = std::find_if(it->second.begin(), it->second.end(), [estid](const order_match* cur) ->bool {
				
				return cur&&cur->order.est_id == estid;
				});
			if (od_it != it->second.end())
			{
				(*od_it)->queue_seat = seat;
			}
		}
	}
}

void order_container::set_state(estid_t estid, order_state state)
{
	spin_lock lock(_mutex);
	auto odit = _order_info.find(estid);
	if (odit != _order_info.end())
	{
		auto it = _order_match.find(odit->second.code);
		if (it != _order_match.end())
		{
			auto od_it = std::find_if(it->second.begin(), it->second.end(), [estid](const order_match* cur) ->bool {

				return cur&&cur->order.est_id == estid;
				});
			if (od_it != it->second.end())
			{
				(*od_it)->state = state;
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
			match_list.emplace_back(*od_it);
		}
	}
}

const order_match* order_container::get_order_match(estid_t estid)const
{
	spin_lock lock(_mutex);
	auto odit = _order_info.find(estid);
	if (odit != _order_info.end())
	{
		auto it = _order_match.find(odit->second.code);
		if (it != _order_match.end())
		{
			auto od_it = std::find_if(it->second.begin(), it->second.end(), [estid](const order_match* cur) ->bool {

				return cur && cur->order.est_id == estid;
				});
			if (od_it != it->second.end())
			{
				return (*od_it);
			}
		}
	}
	return nullptr ;
}

bool order_container::get_order_info(order_info& order, estid_t estid)const
{
	spin_lock lock(_mutex);
	LOG_TRACE("order_container get_order_info", estid, _order_info.size());
	auto odit = _order_info.find(estid);
	if (odit != _order_info.end())
	{
		order = odit->second;
		LOG_TRACE("order_container get_order_info order %lld %d ", order.est_id, _order_info.size());
		return true;
	}
	LOG_TRACE("order_container get_order_info false");
	return false;
}
void order_container::get_all_order(std::vector<order_info>& order)const
{
	spin_lock lock(_mutex);
	for (auto it : _order_match)
	{
		for (auto mh : it.second)
		{
			order.emplace_back(mh->order);
		}
	}
}

void order_container::get_valid_order(std::vector<order_info>& order)const
{
	spin_lock lock(_mutex);
	for (auto it : _order_match)
	{
		for (auto mh : it.second)
		{
			if (mh->state != OS_CANELED && mh->state != OS_INVALID)
			{
				order.emplace_back(mh->order);
			}
		}
	}
}

void order_container::clear()
{
	spin_lock lock(_mutex);
	_order_info.clear();
	for(auto it : _order_match)
	{
		for(auto mh : it.second)
		{
			delete mh;
		}
	}
	_order_match.clear();
}

bool order_container::exist(estid_t estid)
{
	spin_lock lock(_mutex);
	auto it = _order_info.find(estid);
	if(it != _order_info.end())
	{
		return true;
	}
	return false ;
}