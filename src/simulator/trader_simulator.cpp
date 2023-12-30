#include "trader_simulator.h"
#include <data_types.hpp>
#include <event_center.hpp>
#include "contract_parser.h"
#include "./tick_loader/csv_tick_loader.h"
#include <log_wapper.hpp>

trader_simulator::trader_simulator(const params& config) :
	_trading_day(0),
	_order_ref(0),
	_interval(1),
	_current_time(0)
{
	try
	{
		_account_info.money = config.get<double_t>("initial_capital");
		auto contract_file = config.get<std::string>("contract_config");
		_contract_parser.init(contract_file);
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


void trader_simulator::push_tick(const tick_info& tick)
{
	_current_tick_info[tick.id] = tick;
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
			if(it.second.yestoday_long.postion > 0)
			{
				it.second.today_long.price = (it.second.yestoday_long.postion * it.second.yestoday_long.price + it.second.today_long.postion * it.second.today_long.price) / (it.second.yestoday_long.postion + it.second.today_long.postion);
				it.second.today_long.postion += it.second.yestoday_long.postion;
				it.second.yestoday_long.postion = 0;
			} 
			if(it.second.yestoday_short.postion > 0)
			{
				it.second.today_short.price = (it.second.yestoday_short.postion * it.second.yestoday_short.price + it.second.today_short.postion * it.second.today_short.price) / (it.second.yestoday_short.postion + it.second.today_short.postion);
				it.second.today_short.postion += it.second.yestoday_short.postion;
				it.second.yestoday_short.postion = 0;
			}
			it.second.yestoday_long.frozen = 0;
			it.second.yestoday_short.frozen = 0;
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
		match_entrust(&tk_it.second);
		_last_frame_volume[tk_it.second.id] = tk_it.second.volume;
	}
	/*
	double_t frozen_monery = .0;
	for(const auto& it : _order_info)
	{
		bool is_matching = false ;

		auto match = _order_match.find(it.second.code);
		if (match != _order_match.end())
		{
			estid_t estid = it.first;
			auto mch_odr = std::find_if(match->second.begin(), match->second.end(), [estid](const order_match& p)->bool {
				return p.estid == estid;
				});
			if (mch_odr != match->second.end())
			{
				is_matching = mch_odr->state != order_state::OS_INVALID;
			}
		}

		if(!is_matching|| it.second.offset != offset_type::OT_OPEN)
		{
			continue;
		}
		auto contract_info = _contract_parser.get_contract_info(it.second.code);
		if (contract_info == nullptr)
		{
			LOG_ERROR("tick_simulator frozen_deduction cant find the contract_info for", it.second.code.get_id());
			return;
		}
		frozen_monery += (it.second.last_volume * it.second.price * contract_info->multiple * contract_info->margin_rate);
	}
	for (const auto& it : _position_info)
	{
		auto contract_info = _contract_parser.get_contract_info(it.first);
		if (contract_info == nullptr)
		{
			LOG_ERROR("tick_simulator frozen_deduction cant find the contract_info for", it.first.get_id());
			return;
		}
		frozen_monery += it.second.today_long.price * it.second.today_long.postion * contract_info->multiple* contract_info->margin_rate ;
		frozen_monery += it.second.today_short.price * it.second.today_short.postion * contract_info->multiple * contract_info->margin_rate;
		frozen_monery += it.second.yestoday_long.price * it.second.yestoday_long.postion * contract_info->multiple * contract_info->margin_rate;
		frozen_monery += it.second.yestoday_short.price * it.second.yestoday_short.postion * contract_info->multiple * contract_info->margin_rate;
	}
	if(_account_info.frozen_monery - frozen_monery>100|| _account_info.frozen_monery - frozen_monery<-100)
	{
		LOG_ERROR("frozen_monery not match ", _account_info.frozen_monery, frozen_monery);
		return;
	}
	*/
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
	PROFILE_INFO(code.get_id());
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
	LOG_TRACE("order_container add_order", order.code.get_id(), order.estid, _order_info.size());
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


std::shared_ptr<trader_data> trader_simulator::get_trader_data()
{
	auto result = std::make_shared<trader_data>();
	
	for (const auto& it : _order_info)
	{
		result->orders.emplace_back(it.second);
	}
	
	for(const auto& it : _position_info)
	{
		position_seed pos ;
		pos.id = it.first;
		pos.today_long = it.second.today_long.postion;
		pos.today_short = it.second.today_short.postion;
		pos.history_long = it.second.yestoday_long.postion;
		pos.history_short = it.second.yestoday_short.postion;
		result->positions.emplace_back(pos);
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
	auto buy_it = std::find_if(tick.buy_order.begin(), tick.buy_order.end(), [price](const std::pair<double_t, uint32_t>& cur) ->bool {

		return cur.first == price;
		});
	if (buy_it != tick.buy_order.end())
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
	auto sell_it = std::find_if(tick.sell_order.begin(), tick.sell_order.end(), [price](const std::pair<double_t, uint32_t>& cur) ->bool {

		return cur.first == price;
		});
	if (sell_it != tick.sell_order.end())
	{
		return sell_it->second;
	}
	return 0U;
}

void trader_simulator::match_entrust(const tick_info* tick)
{
	auto last_volume = _last_frame_volume.find(tick->id);
	if(last_volume == _last_frame_volume.end())
	{
		return ;
	}
	uint32_t current_volume = static_cast<uint32_t>(tick->volume - last_volume->second);
	auto it = _order_match.find(tick->id);
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
	
}

void trader_simulator::handle_entrust(const tick_info* tick, order_match& match, order_info& order, uint32_t max_volume)
{
	if(match.state == OS_INVALID)
	{
		error_code err = frozen_deduction(order.estid, order.code, order.offset, order.direction, order.last_volume, order.price);
		if (err == error_code::EC_Success)
		{
			this->fire_event(trader_event_type::TET_OrderPlace, order);
			
			visit_match_info(order.estid,[this,&order](order_match& mh)->void{
				if(order.is_buy())
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
		else
		{
			order_error(error_type::ET_PLACE_ORDER,order.estid, err);
		}
		return ;
	}
	if(match.state == OS_CANELED)
	{
		//撤单
		order_cancel(order);
		return;
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
void trader_simulator::handle_sell(const tick_info* tick,order_match& match, order_info& order, uint32_t max_volume)
{

	if (match.flag == order_flag::OF_FOK)
	{
		if (order.last_volume <= max_volume && order.price <= tick->buy_price())
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
		if(order.price <= tick->buy_price())
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
		if (order.price <= tick->buy_price())
		{
			//不需要排队，直接降价成交
			uint32_t deal_volume = order.last_volume > max_volume ? max_volume : order.last_volume;
			if (deal_volume > 0)
			{
				order_deal(order, deal_volume);
			}
		}
		else if (order.price <= tick->price)
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

void trader_simulator::handle_buy(const tick_info* tick, order_match& match, order_info& order, uint32_t max_volume)
{

	if (match.flag == order_flag::OF_FOK)
	{
		if (order.last_volume <= max_volume&& order.price >= tick->sell_price())
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
		if(order.price >= tick->sell_price())
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
		if (order.price >= tick->sell_price())
		{
			//不需要排队，直接降价成交
			uint32_t deal_volume = order.last_volume > max_volume ? max_volume : order.last_volume;
			if (deal_volume > 0)
			{
				order_deal(order, deal_volume);
			}
		}
		else if (order.price >= tick->price)
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
		LOG_ERROR("tick_simulator order_deal cant find the contract_info for", order.code.get_id());
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
				pos.today_long.price = (pos.today_long.postion * pos.today_long.price + order.price * deal_volume) / (pos.today_long.postion + deal_volume);
				pos.today_long.postion += deal_volume;
				_account_info.money -=  deal_volume * service_charge;
			}
			
		}
		else if (order.direction == direction_type::DT_SHORT)
		{
			if(_account_info.money >= deal_volume * service_charge)
			{
				pos.today_short.price = (pos.today_short.postion * pos.today_short.price + order.price * deal_volume) / (pos.today_short.postion + deal_volume);
				pos.today_short.postion += deal_volume;
				_account_info.money -= (deal_volume * service_charge);
			}
		}
	}
	else if (order.offset == offset_type::OT_CLSTD)
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
			_account_info.money += (deal_volume * (order.price - pos.today_long.price) * contract_info->multiple);
			pos.today_long.postion -= std::min<uint32_t>(deal_volume, pos.today_long.postion);
			pos.today_long.frozen -= std::min<uint32_t>(deal_volume, pos.today_long.frozen);
			_account_info.frozen_monery -= (deal_volume * pos.today_long.price * contract_info->multiple * contract_info->margin_rate);
			_account_info.money -= (deal_volume * service_charge);
		}
		else if (order.direction == direction_type::DT_SHORT)
		{
			_account_info.money += (deal_volume * (pos.today_short.price - order.price) * contract_info->multiple);
			pos.today_short.postion -= std::min<uint32_t>(deal_volume, pos.today_short.postion);
			pos.today_short.frozen -= std::min<uint32_t>(deal_volume, pos.today_short.frozen);
			_account_info.frozen_monery -= (deal_volume * pos.today_short.price * contract_info->multiple * contract_info->margin_rate);
			_account_info.money -= deal_volume * service_charge;
		}
	}
	else
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
	
	order.last_volume = (order.estid,order.last_volume - deal_volume);
	//部分成交
	fire_event(trader_event_type::TET_OrderDeal, order.estid, deal_volume, order.total_volume);
	if(order.last_volume == 0)
	{
		LOG_TRACE(" order_deal _order_info.del_order", order.estid);
		//全部成交
		fire_event(trader_event_type::TET_OrderTrade, order.estid, order.code, order.offset, order.direction, order.price, order.total_volume);
		remove_order(order.estid);
	}
	
}
void trader_simulator::order_error(error_type type,estid_t estid, error_code err)
{
	fire_event(trader_event_type::TET_OrderError, type, estid, (uint8_t)err);
	remove_order(estid);
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
			remove_order(order.estid);
		}
		else
		{
			LOG_ERROR("order_cancel error");
		}
	}
}

void trader_simulator::remove_order(estid_t estid)
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
				match->second.erase(mch_odr);
			}
		}
		LOG_INFO("remove_order", estid);
		_order_info.erase(odit);
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
		LOG_ERROR("tick_simulator frozen_deduction cant find the contract_info for", code.get_id());
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
		LOG_ERROR("tick_simulator frozen_deduction cant find the position_info for ", code.get_id());
		return error_code::EC_PositionNotEnough;
	}
	if (offset == offset_type::OT_CLSTD)
	{
		
		if (direction == direction_type::DT_LONG)
		{
			LOG_TRACE("frozen_deduction long today", code.get_id(), estid, it->second.today_long.usable());
			if (it->second.today_long.usable() < volume)
			{
				return error_code::EC_PositionNotEnough;
			}
			it->second.today_long.frozen += volume;
		}
		else if (direction == direction_type::DT_SHORT)
		{
			LOG_TRACE("frozen_deduction short today", code.get_id(), estid, it->second.today_short.usable());
			if (it->second.today_short.usable() < volume)
			{
				return error_code::EC_PositionNotEnough;
			}
			it->second.today_short.frozen += volume;
		}
		return error_code::EC_Success;
	}
	else
	{
		if (direction == direction_type::DT_LONG)
		{
			LOG_TRACE("frozen_deduction long yestoday", code.get_id(), estid, it->second.yestoday_long.usable());
			if (it->second.yestoday_long.usable() < volume)
			{
				return error_code::EC_PositionNotEnough;
			}
			it->second.yestoday_long.frozen += volume;

		}
		else if (direction == direction_type::DT_SHORT)
		{
			LOG_TRACE("frozen_deduction short yestoday", code.get_id(), estid, it->second.yestoday_short.usable());
			if (it->second.yestoday_short.usable() < volume)
			{
				return error_code::EC_PositionNotEnough;
			}
			it->second.yestoday_short.frozen += volume;
		}
		return error_code::EC_Success;
	}
	return error_code::EC_OrderFieldError;
}
bool trader_simulator::unfrozen_deduction(const code_t& code, offset_type offset, direction_type direction, uint32_t last_volume, double_t price)
{
	auto contract_info = _contract_parser.get_contract_info(code);
	if (contract_info == nullptr)
	{
		LOG_ERROR("tick_simulator frozen_deduction cant find the contract_info for ", code.get_id());
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
		LOG_ERROR("tick_simulator frozen_deduction cant find the position_info for ", code.get_id());
		return false;
	}
	if (offset == offset_type::OT_CLSTD)
	{

		if (direction == direction_type::DT_LONG)
		{
			it->second.today_long.frozen -= std::min<uint32_t>(last_volume, it->second.today_long.frozen);
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.today_short.frozen -= std::min<uint32_t>(last_volume, it->second.today_short.frozen);
		}
	}
	else
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
