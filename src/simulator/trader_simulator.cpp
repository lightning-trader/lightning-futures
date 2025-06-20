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
#include "trader_simulator.h"
#include <event_center.hpp>
#include "contract_parser.h"
#include <log_define.hpp>

using namespace lt;
using namespace lt::driver;

trader_simulator::trader_simulator(const params& config) :
	_trading_day(0),
	_order_ref(0),
	_interval(config.get<uint32_t>("interval")),
	_current_time(0),
	_contract_parser(config.get<std::string>("contract_config").c_str())
{
	try
	{
		_account_info.money = config.get<double_t>("initial_capital");
		_interval = config.get<uint32_t>("interval");
	}
	catch (...)
	{
		LOG_ERROR("tick_simulator init error ");
	}

}
trader_simulator::~trader_simulator()
{
}


void trader_simulator::push_tick(const std::vector<const tick_info*>& current_tick)
{
	for(auto tick : current_tick)
	{
		if(tick)
		{
			_current_tick_info[tick->id] = *tick;
		}
	}
	
}

void trader_simulator::crossday(uint32_t trading_day)
{
	_trading_day = trading_day;
	std::vector<order_info> order;
	for (auto& it : _order_info)
	{
		cancel_order(it.second.estid);
	}
	for (auto& it : _position_info)
	{
		if (it.first.is_distinct())
		{
			it.second.total_long.frozen = 0;
			it.second.total_short.frozen = 0;
			it.second.yestoday_long = it.second.total_long;
			it.second.yestoday_short = it.second.total_short;
		}
	}
	
}

uint32_t trader_simulator::get_trading_day()const
{
	return _trading_day;
}

void trader_simulator::update()
{
	for (const auto& tk_it : _current_tick_info)
	{
		_current_time = tk_it.second.time;
		match_entrust(tk_it.second);
		_last_frame_volume[tk_it.second.id] = tk_it.second.volume;
	}

}

bool trader_simulator::is_usable()const
{
	return true ;
}

estid_t trader_simulator::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	
	//boost::posix_time::ptime pt2 = boost::posix_time::microsec_clock::local_time();
	//std::cout << "place_order : " << pt2 - pt << estid.to_str() << std::endl;
	
	//spin_lock lock(_mutex);
	PROFILE_INFO(code.get_symbol());
	order_info order;
	order.estid = make_estid();
	LOG_DEBUG("tick_simulator::place_order", order.estid);
	order.code = code ;
	order.create_time = _current_time;
	order.offset = offset;
	order.direction = direction;
	order.total_volume = count;
	order.last_volume = count;
	if (price == .0F)
	{
		auto tick_it = _current_tick_info.find(code);
		if (tick_it != _current_tick_info.end())
		{
			order.price = tick_it->second.price;
		}
	}
	else
	{
		order.price = price;
	}
	_order_info[order.estid] = order;
	LOG_TRACE("order_container add_order", order.code.get_symbol(), order.estid, _order_info.size());
	_order_match[order.code].emplace_back(order_match(order.estid, flag));
	return order.estid;
}

bool trader_simulator::cancel_order(estid_t estid)
{
	LOG_DEBUG("tick_simulator cancel_order", estid);
	auto odit = _order_info.find(estid);
	if (odit == _order_info.end())
	{
		return false;
	}
	auto it = _order_match.find(odit->second.code);
	if (it == _order_match.end())
	{
		return false;
	}
	auto od_it = std::find_if(it->second.begin(), it->second.end(), [estid](const order_match& cur) ->bool {

		return cur.estid == estid;
		});
	if (od_it == it->second.end())
	{
		return false;
	}
	if ((od_it)->state == OS_INVALID)
	{
		return false;
	}
	if ((od_it)->state != OS_CANELED)
	{
		(od_it)->state = OS_CANELED;
	}
	return true;
}

std::vector<order_info> trader_simulator::get_all_orders()
{
	std::vector<order_info> result;

	for (const auto& it : _order_info)
	{
		result.emplace_back(it.second);
	}
	return result;
}

std::vector<position_seed> trader_simulator::get_all_positions()
{
	std::vector<position_seed> result;
	for (const auto& it : _position_info)
	{
		position_seed pos;
		pos.id = it.first;
		pos.total_long = it.second.total_long.postion;
		pos.total_short = it.second.total_short.postion;
		pos.history_long = it.second.yestoday_long.postion;
		pos.history_short = it.second.yestoday_short.postion;
		result.emplace_back(pos);
	}
	return result;
}

std::vector<instrument_info> trader_simulator::get_all_instruments()
{
	std::vector<instrument_info> result;
	const auto & all_contract = _contract_parser.get_all_contract();
	for(const auto & it : all_contract)
	{
		//contract_info info;
		//info.code = it.second.code;
		//info.multiple = it.second.multiple;
		//info.price_step = it.second.price_step;
		result.emplace_back(it.second);
	}
	return result;


}


estid_t trader_simulator::make_estid()
{
	_order_ref++;
	uint64_t p1 = (uint64_t)_current_time<<32;
	uint64_t p2 = (uint64_t)0<<16;
	uint64_t p3 = (uint64_t)_order_ref;

	uint64_t v1 = p1 & 0xFFFFFFFF00000000LLU;
	uint64_t v2 = p2 & 0x00000000FFFF0000LLU;
	uint64_t v3 = p3 & 0x000000000000FFFFLLU;
	return v1 + v2 + v3;
}

uint32_t trader_simulator::get_buy_front(const code_t& code,double_t price)
{
	auto tick_it = _current_tick_info.find(code);
	if(tick_it == _current_tick_info.end())
	{
		return 0U;
	}
	const auto& tick = tick_it->second;
	auto buy_it = std::find_if(tick.bid_order.begin(), tick.bid_order.end(), [price](const std::pair<double_t, uint32_t>& cur) ->bool {

		return cur.first == price;
		});
	if (buy_it != tick.bid_order.end())
	{
		return buy_it->second;
	}

	return 0U;
}
uint32_t trader_simulator::get_sell_front(const code_t& code, double_t price)
{
	auto tick_it = _current_tick_info.find(code);
	if (tick_it == _current_tick_info.end())
	{
		return 0U;
	}
	const auto& tick = tick_it->second;
	auto sell_it = std::find_if(tick.ask_order.begin(), tick.ask_order.end(), [price](const std::pair<double_t, uint32_t>& cur) ->bool {

		return cur.first == price;
		});
	if (sell_it != tick.ask_order.end())
	{
		return sell_it->second;
	}
	return 0U;
}

void trader_simulator::match_entrust(const tick_info& tick)
{
	uint32_t current_volume = static_cast<uint32_t>(tick.volume);
	auto last_volume = _last_frame_volume.find(tick.id);
	if(last_volume != _last_frame_volume.end())
	{
		current_volume = static_cast<uint32_t>(tick.volume - last_volume->second);
	}
	auto it = _order_match.find(tick.id);
	if (it != _order_match.end())
	{
		for (order_match& mc_it : it->second)
		{
			auto od_it = _order_info.find(mc_it.estid);
			if(od_it != _order_info.end())
			{
				handle_entrust(tick, mc_it, od_it->second, current_volume);
			}
		}
	}
	
	for(auto mc_it = _order_match.begin();mc_it != _order_match.end();){
		for(auto it = mc_it->second.begin();it!= mc_it->second.end();){
			if(it->state == OS_DELETE)
			{
				auto odit = _order_info.find(it->estid);
				if (odit != _order_info.end())
				{

					LOG_INFO("remove_order", it->estid);
					_order_info.erase(odit);
				}
				it = mc_it->second.erase(it);
			}
			else
			{
				++it;
			}
		}
		if(mc_it->second.empty())
		{
			mc_it = _order_match.erase(mc_it);
		}
		else
		{
			mc_it++;
		}
	}
}

void trader_simulator::handle_entrust(const tick_info& tick, order_match& match, order_info& order, uint32_t max_volume)
{
	if (match.state == OS_CANELED)
	{
		//撤单
		order_cancel(order);
		return;
	}
	if(match.state == OS_INVALID)
	{
		error_code err = frozen_deduction(order.estid, order.code, order.offset, order.direction, order.last_volume, order.price);
		if (err != error_code::EC_Success)
		{
			order_error(error_type::ET_PLACE_ORDER,order.estid, err);
			return;
		}
		this->fire_event(trader_event_type::TET_OrderPlace, order);

		visit_match_info(order.estid, [this, &order](order_match& mh)->void {
			if (order.is_buy())
			{
				mh.queue_seat = this->get_buy_front(order.code, order.price);
			}
			else if (order.is_sell())
			{
				mh.queue_seat = this->get_sell_front(order.code, order.price);
			}
			mh.state = OS_IN_MATCH;
		});
	}

	if (order.direction == direction_type::DT_LONG)
	{	
		if(order.offset == offset_type::OT_OPEN)
		{
			handle_buy(tick, match, order, max_volume);
		}
		else
		{
			handle_sell(tick, match, order, max_volume);
		}
		
	}
	else if (order.direction == direction_type::DT_SHORT)
	{
		if (order.offset == offset_type::OT_OPEN)
		{
			handle_sell(tick, match, order, max_volume);
		}
		else
		{
			handle_buy(tick, match, order, max_volume);
		}
	}
}
void trader_simulator::handle_sell(const tick_info& tick,order_match& match, order_info& order, uint32_t max_volume)
{

	if (match.flag == order_flag::OF_FOK)
	{
		if (order.last_volume <= max_volume && order.price <= tick.buy_price())
		{
			//全成
			order_deal(order, order.last_volume);
		}
		else
		{
			//全撤
			order_cancel(order);
		}
	}
	else if (match.flag == order_flag::OF_FAK)
	{
		if(order.price <= tick.buy_price())
		{
			//部成
			uint32_t deal_volume = order.last_volume > max_volume ? max_volume : order.last_volume;
			if (deal_volume > 0)
			{
				order_deal(order, deal_volume);
			}
			uint32_t cancel_volume = order.last_volume - max_volume;
			if (cancel_volume > 0)
			{
				//部撤
				order_cancel(order);
			}
		}
		else
		{
			order_cancel(order);
		}
	}
	else
	{
		if (order.price <= tick.buy_price())
		{
			//不需要排队，直接降价成交
			uint32_t deal_volume = order.last_volume > max_volume ? max_volume : order.last_volume;
			if (deal_volume > 0)
			{
				order_deal(order, deal_volume);
			}
		}
		else if (order.price <= tick.price)
		{
			//排队成交，移动排队位置
			int32_t new_seat = match.queue_seat - max_volume;
			if (new_seat < 0)
			{
				//排队到了，有成交了
				//deal_count = - new_seat
				match.queue_seat = 0 ;
				uint32_t can_deal_volume = static_cast<uint32_t>(-new_seat);
				uint32_t deal_volume = order.last_volume > can_deal_volume ? can_deal_volume : order.last_volume;
				if (deal_volume > 0U)
				{
					order_deal(order, deal_volume);
				}
			}
			else
			{
				match.queue_seat = new_seat;
			}
		}
	}

	

}

void trader_simulator::handle_buy(const tick_info& tick, order_match& match, order_info& order, uint32_t max_volume)
{

	if (match.flag == order_flag::OF_FOK)
	{
		if (order.last_volume <= max_volume&& order.price >= tick.sell_price())
		{
			//全成
			order_deal(order, order.last_volume);
		}
		else
		{
			//全撤
			order_cancel(order);
		}
	}
	else if (match.flag == order_flag::OF_FAK)
	{
		//部成
		if(order.price >= tick.sell_price())
		{
			uint32_t deal_volume = order.last_volume > max_volume ? max_volume : order.last_volume;
			if (deal_volume > 0U)
			{
				order_deal(order, deal_volume);
			}
			uint32_t cancel_volume = order.last_volume - max_volume;
			if (cancel_volume > 0)
			{
				//部撤
				order_cancel(order);
			}
		}
		else
		{
			order_cancel(order);
		}
		
	}
	else
	{
		//剩下都不是第一帧自动撤销的订单
		if (order.price >= tick.sell_price())
		{
			//不需要排队，直接降价成交
			uint32_t deal_volume = order.last_volume > max_volume ? max_volume : order.last_volume;
			if (deal_volume > 0)
			{
				order_deal(order, deal_volume);
			}
		}
		else if (order.price >= tick.price)
		{
			//有排队的情况
			//排队成交，移动排队位置
			int32_t new_seat = match.queue_seat - max_volume;
			if (new_seat < 0)
			{
				//排队到了，有成交了
				//deal_count = - new_seat
				match.queue_seat = 0 ;
				uint32_t can_deal_volume = static_cast<uint32_t>(-new_seat);
				uint32_t deal_volume = order.last_volume > can_deal_volume ? can_deal_volume : order.last_volume;
				if (deal_volume > 0)
				{
					order_deal(order, deal_volume);
				}
			}
			else
			{
				match.queue_seat = new_seat;
			}
		}
	}
}

void trader_simulator::order_deal(order_info& order, uint32_t deal_volume)
{
	
	auto contract_info = _contract_parser.get_contract_info(order.code);
	if(contract_info == nullptr)
	{
		LOG_ERROR("tick_simulator order_deal cant find the contract_info for", order.code.get_symbol());
		return;
	}

	double_t service_charge = contract_info->get_service_charge(order.price, order.offset);
	if(order.offset == offset_type::OT_OPEN)
	{
		auto& pos = _position_info[order.code];
		//开仓
		if(order.direction == direction_type::DT_LONG)
		{
			
			if(_account_info.money >= deal_volume * service_charge)
			{
				pos.total_long.price = (pos.total_long.postion * pos.total_long.price + order.price * deal_volume) / (pos.total_long.postion + deal_volume);
				pos.total_long.postion += deal_volume;
				_account_info.money -=  deal_volume * service_charge;
			}
			
		}
		else if (order.direction == direction_type::DT_SHORT)
		{
			if(_account_info.money >= deal_volume * service_charge)
			{
				pos.total_short.price = (pos.total_short.postion * pos.total_short.price + order.price * deal_volume) / (pos.total_short.postion + deal_volume);
				pos.total_short.postion += deal_volume;
				_account_info.money -= (deal_volume * service_charge);
			}
		}
	}
	else 
	{
		auto it = _position_info.find(order.code);
		if (it == _position_info.end())
		{
			return;
		}
		auto& pos = it->second ;
		//平仓
		if (order.direction == direction_type::DT_LONG)
		{
			_account_info.money += (deal_volume * (order.price - pos.total_long.price) * contract_info->multiple);
			pos.total_long.postion -= std::min<uint32_t>(deal_volume, pos.total_long.postion);
			pos.total_long.frozen -= std::min<uint32_t>(deal_volume, pos.total_long.frozen);
			_account_info.frozen_monery -= (deal_volume * pos.total_long.price * contract_info->multiple * contract_info->margin_rate);
			_account_info.money -= (deal_volume * service_charge);
		}
		else if (order.direction == direction_type::DT_SHORT)
		{
			_account_info.money += (deal_volume * (pos.total_short.price - order.price) * contract_info->multiple);
			pos.total_short.postion -= std::min<uint32_t>(deal_volume, pos.total_short.postion);
			pos.total_short.frozen -= std::min<uint32_t>(deal_volume, pos.total_short.frozen);
			_account_info.frozen_monery -= (deal_volume * pos.total_short.price * contract_info->multiple * contract_info->margin_rate);
			_account_info.money -= deal_volume * service_charge;
		}
		if (order.offset == offset_type::OT_CLOSE)
		{
			auto it = _position_info.find(order.code);
			if (it == _position_info.end())
			{
				return;
			}
			auto& pos = it->second;
			//平仓
			if (order.direction == direction_type::DT_LONG)
			{
				_account_info.money += (deal_volume * (order.price - pos.yestoday_long.price) * contract_info->multiple);
				pos.yestoday_long.postion -= std::min<uint32_t>(deal_volume, pos.yestoday_long.postion);
				pos.yestoday_long.frozen -= std::min<uint32_t>(deal_volume, pos.yestoday_long.frozen);
				_account_info.frozen_monery -= (deal_volume * pos.yestoday_long.price * contract_info->multiple * contract_info->margin_rate);
				_account_info.money -= (deal_volume * service_charge);
			}
			else if (order.direction == direction_type::DT_SHORT)
			{
				_account_info.money += (deal_volume * (pos.yestoday_short.price - order.price) * contract_info->multiple);
				pos.yestoday_short.postion -= std::min<uint32_t>(deal_volume, pos.yestoday_short.postion);
				pos.yestoday_short.frozen -= std::min<uint32_t>(deal_volume, pos.yestoday_short.frozen);
				_account_info.frozen_monery -= (deal_volume * pos.yestoday_short.price * contract_info->multiple * contract_info->margin_rate);
				_account_info.money -= deal_volume * service_charge;
			}
		}
	}
	
	
	order.last_volume = (order.estid,order.last_volume - deal_volume);
	//部分成交
	fire_event(trader_event_type::TET_OrderDeal, order.estid, deal_volume, order.last_volume);
	if(order.last_volume == 0)
	{
		LOG_TRACE(" order_deal _order_info.del_order", order.estid);
		//全部成交
		fire_event(trader_event_type::TET_OrderTrade, order.estid, order.code, order.offset, order.direction, order.price, order.total_volume);
		visit_match_info(order.estid, [this](order_match& mh)->void {
			mh.state = OS_DELETE;
			});
		
	}
	
}
void trader_simulator::order_error(error_type type,estid_t estid, error_code err)
{
	fire_event(trader_event_type::TET_OrderError, type, estid, (uint8_t)err);
	visit_match_info(estid, [this](order_match& mh)->void {
		mh.state = OS_DELETE;
		});
}
void trader_simulator::order_cancel(const order_info& order)
{
	auto it = _order_info.find(order.estid);
	if(it == _order_info.end())
	{
		return;
	}
	if(order.last_volume>0)
	{
		if(unfrozen_deduction(order.code, order.offset, order.direction, order.last_volume, order.price))
		{
			LOG_INFO(" order_cancel _order_info.del_order", order.estid);
			fire_event(trader_event_type::TET_OrderCancel, order.estid, order.code, order.offset, order.direction, order.price, order.last_volume, order.total_volume);
			visit_match_info(order.estid, [this](order_match& mh)->void {
				mh.state = OS_DELETE;
				});
		}
		else
		{
			LOG_ERROR("order_cancel error");
		}
	}
}


void trader_simulator::visit_match_info(estid_t estid,std::function<void(order_match&)> cursor)
{
	auto odit = _order_info.find(estid);
	if (odit != _order_info.end())
	{
		auto match = _order_match.find(odit->second.code);
		if (match != _order_match.end())
		{
			auto mch_odr = std::find_if(match->second.begin(), match->second.end(), [estid](const order_match& p)->bool {
				return p.estid == estid;
				});
			if (mch_odr != match->second.end())
			{
				cursor(*mch_odr);
			}
		}
	}
}

error_code trader_simulator::frozen_deduction(estid_t estid,const code_t& code,offset_type offset, direction_type direction,uint32_t volume,double_t price)
{
	auto contract_info = _contract_parser.get_contract_info(code);
	if (contract_info == nullptr)
	{
		LOG_ERROR("tick_simulator frozen_deduction cant find the contract_info for", code.get_symbol());
		return error_code::EC_Failure;
	}
	if (offset == offset_type::OT_OPEN)
	{
		double_t frozen_monery = (volume * price * contract_info->multiple * contract_info->margin_rate);
		if (frozen_monery + _account_info.frozen_monery > _account_info.money)
		{
			return error_code::EC_MarginNotEnough;
		}
		//开仓 冻结保证金
		_account_info.frozen_monery += frozen_monery;
		return error_code::EC_Success;
	}
	auto it = _position_info.find(code);
	if (it == _position_info.end())
	{
		LOG_ERROR("tick_simulator frozen_deduction cant find the position_info for ", code.get_symbol());
		return error_code::EC_PositionNotEnough;
	}
	if (direction == direction_type::DT_LONG)
	{
		LOG_TRACE("frozen_deduction long today", code.get_symbol(), estid, it->second.total_long.usable());
		if (it->second.total_long.usable() < volume)
		{
			return error_code::EC_PositionNotEnough;
		}
		it->second.total_long.frozen += volume;
	}
	else if (direction == direction_type::DT_SHORT)
	{
		LOG_TRACE("frozen_deduction short today", code.get_symbol(), estid, it->second.total_short.usable());
		if (it->second.total_short.usable() < volume)
		{
			return error_code::EC_PositionNotEnough;
		}
		it->second.total_short.frozen += volume;
	}
	if (offset == offset_type::OT_CLOSE)
	{

		if (direction == direction_type::DT_LONG)
		{
			LOG_TRACE("frozen_deduction long yestoday", code.get_symbol(), estid, it->second.yestoday_long.usable());
			if (it->second.yestoday_long.usable() < volume)
			{
				return error_code::EC_PositionNotEnough;
			}
			it->second.yestoday_long.frozen += volume;

		}
		else if (direction == direction_type::DT_SHORT)
		{
			LOG_TRACE("frozen_deduction short yestoday", code.get_symbol(), estid, it->second.yestoday_short.usable());
			if (it->second.yestoday_short.usable() < volume)
			{
				return error_code::EC_PositionNotEnough;
			}
			it->second.yestoday_short.frozen += volume;
		}
	}
	return error_code::EC_Success;
}
bool trader_simulator::unfrozen_deduction(const code_t& code, offset_type offset, direction_type direction, uint32_t last_volume, double_t price)
{
	auto contract_info = _contract_parser.get_contract_info(code);
	if (contract_info == nullptr)
	{
		LOG_ERROR("tick_simulator frozen_deduction cant find the contract_info for ", code.get_symbol());
		return false;
	}

	//double_t service_charge = contract_info->get_service_charge(price, offset, is_today);
	//_account_info.money+= last_volume * service_charge;
	if (offset == offset_type::OT_OPEN)
	{
		double_t delta = (last_volume * price * contract_info->multiple * contract_info->margin_rate);
		//撤单 取消冻结保证金
		LOG_TRACE("thawing_deduction 1", _account_info.frozen_monery, delta, last_volume , price);
		_account_info.frozen_monery -= std::min<double_t>(delta, _account_info.frozen_monery);
		LOG_TRACE("thawing_deduction 2", _account_info.frozen_monery, delta);
		return true ;
	}
	auto it = _position_info.find(code);
	if (it == _position_info.end())
	{
		LOG_ERROR("tick_simulator frozen_deduction cant find the position_info for ", code.get_symbol());
		return false;
	}
	if (direction == direction_type::DT_LONG)
	{
		it->second.total_long.frozen -= std::min<uint32_t>(last_volume, it->second.total_long.frozen);
	}
	else if (direction == direction_type::DT_SHORT)
	{
		it->second.total_short.frozen -= std::min<uint32_t>(last_volume, it->second.total_short.frozen);
	}
	if (offset == offset_type::OT_CLOSE)
	{
		if (direction == direction_type::DT_LONG)
		{
			it->second.yestoday_long.frozen -= std::min<uint32_t>(last_volume, it->second.yestoday_long.frozen);
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.yestoday_short.frozen -= std::min<uint32_t>(last_volume, it->second.yestoday_short.frozen);
		}
	}
	return true;
}
