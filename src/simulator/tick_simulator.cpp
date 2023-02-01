#include "tick_simulator.h"
#include <data_types.hpp>
#include <event_center.hpp>
#include "./tick_loader/csv_tick_loader.h"
#include <boost/date_time/posix_time/posix_time.hpp>

bool tick_simulator::init(const boost::property_tree::ptree& config)
{
	std::string loader_type ;
	std::string csv_data_path ;
	try
	{
		_account_info.money = config.get<double_t>("initial_capital");
		_service_charge = config.get<double_t>("service_charge");
		_multiple = config.get<uint32_t>("multiple");
		_margin_rate = config.get<double_t>("margin_rate");
		_interval = config.get<uint32_t>("interval",1);
		_compulsory_factor = config.get<double_t>("compulsory_factor", 0.8);
		loader_type = config.get<std::string>("loader_type");
		csv_data_path = config.get<std::string>("csv_data_path");
	}
	catch (...)
	{
		LOG_ERROR("tick_simulator init error ");
		return false;
	}
	if (loader_type == "csv")
	{
		csv_tick_loader* loader = new csv_tick_loader();
		if(!loader->init(csv_data_path))
		{
			delete loader;
			return false ;
		}
		_loader = loader;
	}
	return true;
}

void tick_simulator::play(uint32_t tradeing_day)
{
	_current_time = 0;
	_current_tick = 0;
	_current_index = 0;
	_pending_tick_info.clear();
	for (auto& it : _instrument_id_list)
	{
		load_data(it, tradeing_day);
	}
	//模拟跨天时候之前的订单失效
	_order_info.clear();
	
	for(auto it : _position_info)
	{
		auto pit = _standard_price.find(it.first);
		if(pit != _standard_price.end())
		{
			double_t standard_price = pit->second;
			double_t delta_today_long_monery = (standard_price - it.second.today_long.price) * _multiple * it.second.today_long.postion;
			double_t delta_today_short_monery = (it.second.today_short.price - standard_price) * _multiple * it.second.today_short.postion;
			double_t delta_yestoday_long_monery = (standard_price - it.second.yestoday_long.price) * _multiple * it.second.yestoday_long.postion;
			double_t delta_yestoday_short_monery = (it.second.yestoday_short.price - standard_price) * _multiple * it.second.yestoday_short.postion;
			double_t delta_today_long_margin = (it.second.today_long.price - standard_price) * _multiple * it.second.today_long.postion * _margin_rate;
			double_t delta_today_short_margin = (it.second.today_short.price - standard_price) * _multiple * it.second.today_short.postion * _margin_rate;
			double_t delta_yestoday_long_margin = (it.second.yestoday_long.price - standard_price) * _multiple * it.second.yestoday_long.postion * _margin_rate;
			double_t delta_yestoday_short_margin = (it.second.today_short.price - standard_price) * _multiple * it.second.yestoday_long.postion * _margin_rate;
			it.second.yestoday_long.postion = it.second.today_long.postion;
			it.second.yestoday_long.price = standard_price;
			it.second.yestoday_long.frozen = 0;
			it.second.yestoday_short.postion = it.second.today_short.postion;
			it.second.yestoday_short.price = standard_price;
			it.second.yestoday_short.frozen = 0;
			it.second.today_long.clear();
			it.second.today_short.clear();

			_account_info.money += (delta_today_long_monery + delta_today_short_monery + delta_yestoday_long_monery + delta_yestoday_short_monery);
			_account_info.frozen_monery -= (delta_today_long_margin + delta_today_short_margin+ delta_yestoday_long_margin + delta_yestoday_short_margin);
		}
		
	}
	

	_is_in_trading = true ;
	while (_is_in_trading)
	{
		handle_submit();
		//先触发tick，再进行撮合
		publish_tick();
		handle_order();
		//处理强平
		compulsory_closing();
		//std::chrono::milliseconds(_interval)
		std::this_thread::sleep_for(std::chrono::microseconds(_interval));

		_last_frame_volume.clear();
		for (const auto& tick : _current_tick_info)
		{
			_last_frame_volume[tick->id] = tick->volume;
		}
	}
}


void tick_simulator::subscribe(const std::set<code_t>& codes)
{
	for(auto& it : codes)
	{
		_instrument_id_list.insert(it);
	}
}

void tick_simulator::unsubscribe(const std::set<code_t>& codes)
{
	for (auto& it : codes)
	{
		auto cur = _instrument_id_list.find(it);
		if(cur != _instrument_id_list.end())
		{
			_instrument_id_list.erase(cur);
		}
	}
}

time_t tick_simulator::last_tick_time()const
{
	return _current_time;
}


uint32_t tick_simulator::get_trading_day()const
{
	return _current_trading_day;
}

bool tick_simulator::is_usable()const
{
	return true ;
}

estid_t tick_simulator::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	
	//boost::posix_time::ptime pt2 = boost::posix_time::microsec_clock::local_time();
	//std::cout << "place_order : " << pt2 - pt << est_id.to_str() << std::endl;
	//LOG_DEBUG("tick_simulator::place_order 1 %s \n", est_id.to_str());

	//spin_lock lock(_mutex);
	

	order_info order;
	order.est_id = make_estid();
	LOG_DEBUG("tick_simulator::place_order %llu \n", order.est_id);
	order.code = code ;
	order.create_time = last_tick_time();
	order.offset = offset;
	order.direction = direction;
	order.total_volume = count;
	order.last_volume = count;
	order.price = price;
	_order_info.add_order(order,flag);
	return order.est_id;
}

void tick_simulator::cancel_order(estid_t order_id)
{
	LOG_DEBUG("tick_simulator cancel_order %lld", order_id);
	auto match = _order_info.get_order_match(order_id);
	if(match)
	{
		if(match->state == OS_INVALID)
		{
			return;
		}
		if(match->state != OS_CANELED)
		{
			_order_info.set_state(order_id, OS_CANELED);
		}
	}
	
}

const account_info& tick_simulator::get_account() const
{
	return _account_info ;
}

const position_info& tick_simulator::get_position(const code_t& code) const
{
	auto it = _position_info.find(code);
	if(it != _position_info.end())
	{
		return (it->second);
	}
	return default_position;
}

uint32_t tick_simulator::get_total_position()const
{
	uint32_t total = 0;
	for (const auto& it : _position_info)
	{
		total += it.second.get_total();
	}
	return total;
}

const order_info& tick_simulator::get_order(estid_t order_id) const
{
	static order_info order ;
	_order_info.get_order_info(order,order_id);
	LOG_TRACE("tick_simulator get_order  %lld %s ",order.est_id, order.code.get_id());
	return std::move(order);
}

void tick_simulator::find_orders(std::vector<order_info>& order_result, std::function<bool(const order_info&)> func) const
{
	std::vector<order_info> all_order;
	_order_info.get_all_order(all_order);
	for (auto& it : all_order)
	{
		if (func(it))
		{
			order_result.emplace_back(it);
		}
	}
}

void tick_simulator::submit_settlement()
{
	while (!_is_submit_return.exchange(false));
}

bool tick_simulator::get_instrument(const code_t& codes)
{
	return false ;
}

bool tick_simulator::is_in_trading(const code_t& code)
{
	return true ;
}

void tick_simulator::load_data(const code_t& code, uint32_t trading_day)
{
	if(_loader)
	{
		_loader->load_tick(_pending_tick_info,code, trading_day);
		if(!_pending_tick_info.empty())
		{
			_standard_price[code] = _pending_tick_info[0].standard;
		}
	}
}

void tick_simulator::handle_submit()
{
	if (!_is_submit_return)
	{
		while(!_is_submit_return.exchange(true));
		this->fire_event(ET_TradingReady);
	}
}

void tick_simulator::publish_tick()
{	
	const tick_info* tick = nullptr;
	if (_current_index < _pending_tick_info.size())
	{
		tick = &(_pending_tick_info[_current_index]);
		_current_time = tick->time;
		_current_tick = tick->tick;
	}
	else
	{
		//结束了触发收盘事件
		_is_in_trading = false;
		_last_frame_volume.clear();
		return;
	}
	_current_tick_info.clear();
	while(_current_time == tick->time && _current_tick == tick->tick)
	{
		if(tick->trading_day != _current_trading_day)
		{
			_current_trading_day = tick->trading_day;
			
			this->fire_event(ET_CrossDay, _current_trading_day);
		}
		_current_tick_info.emplace_back(tick);
		this->fire_event(ET_TickReceived, *tick);

		_current_index++;
		if(_current_index < _pending_tick_info.size())
		{
			tick = &(_pending_tick_info[_current_index]);
			
		}
		else
		{
			//结束了触发收盘事件
			_is_in_trading = false ;
			_last_frame_volume.clear();
			return;
		}
	}
}

void tick_simulator::handle_order()
{
	for(const auto& tick : _current_tick_info)
	{
		match_entrust(tick);
	}
	
}

void tick_simulator::compulsory_closing()
{
	/*
	double_t delta_money = .0F;
	for(auto& it : _position_info)
	{
		auto& pos = it.second;
		auto tick_it = std::find_if(_current_tick_info.begin(), _current_tick_info.end(),[it](const tick_info* p)->bool{
			return p->id == it.first;
		});
		if(tick_it != _current_tick_info.end())
		{
			double_t current_price = (*tick_it)->price;
			delta_money += pos.long_postion * (current_price - pos.buy_price) * _multiple;
			delta_money += pos.short_postion * (pos.sell_price - current_price) * _multiple;
		}
	}
	//权益资金小于现有资金的20%触发强平
	if(_account_info.money + delta_money< _account_info.frozen_monery * _compulsory_factor)
	{
		for (auto& it : _position_info)
		{
			auto& pos = it.second;
			auto tick_it = std::find_if(_current_tick_info.begin(), _current_tick_info.end(), [it](const tick_info* p)->bool {
				return p->id == it.first;
				});
			if (tick_it != _current_tick_info.end())
			{
				double_t buy_price = (*tick_it)->buy_price();
				double_t sell_price = (*tick_it)->sell_price();
				//假定市价单全部能成交，暂时不考虑穿仓情况
				//强平多仓
				_account_info.money += pos.long_postion * (sell_price - pos.buy_price) * _multiple;
				_account_info.money -= pos.long_postion * _service_charge;
				_account_info.frozen_monery -= pos.long_postion * sell_price * _multiple * _margin_rate;
				pos.long_postion -= pos.long_postion;
				pos.long_frozen -= pos.long_postion;
				//强平空仓
				_account_info.money += pos.short_postion * (pos.sell_price - buy_price) * _multiple;
				_account_info.money -= pos.short_postion * _service_charge;
				_account_info.frozen_monery -= pos.short_postion * pos.sell_price * _multiple * _margin_rate;
				pos.short_postion -= pos.short_postion;
				pos.short_frozen -= pos.short_postion;
				//强平 以后所有平仓订单失效
				std::vector<order_match> match_list;
				_order_info.get_order_match(match_list, pos.id);
				for (auto& mh_it : match_list)
				{
					if (mh_it.state == OS_IN_MATCH)
					{
						order_cancel(mh_it.order);
					}
					else
					{
						order_error(ET_PLACE_ORDER,mh_it.order.est_id, 30U);
					}
				}
			}
		}
		
	}
	*/
}

estid_t tick_simulator::make_estid()
{
	_order_ref++;
	uint64_t p1 = (uint64_t)_current_time<<32;
	uint64_t p2 = (uint64_t)_current_tick<<16;
	uint64_t p3 = (uint64_t)_order_ref;

	uint64_t v1 = p1 & 0xFFFFFFFF00000000LLU;
	uint64_t v2 = p2 & 0x00000000FFFF0000LLU;
	uint64_t v3 = p3 & 0x000000000000FFFFLLU;
	return v1 + v2 + v3;
}

uint32_t tick_simulator::get_front_count(const code_t& code,double_t price)
{
	auto tick_it = std::find_if(_current_tick_info.begin(), _current_tick_info.end(),[code](auto cur) ->bool {
		if(cur->id == code)
		{
			return true ;
		}
		return false ;
	});
	if(tick_it != _current_tick_info.end())
	{
		const auto& tick = *tick_it;
		auto buy_it = std::find_if(tick->buy_order.begin(), tick->buy_order.end(), [price](const std::pair<double_t,uint32_t>& cur) ->bool {
			
			return cur.first == price;
			});
		if(buy_it != tick->buy_order.end())
		{
			return buy_it->second ;
		}
		auto sell_it = std::find_if(tick->sell_order.begin(), tick->sell_order.end(), [price](const std::pair<double_t, uint32_t>& cur) ->bool {
			
			return cur.first == price;
			});
		if (sell_it != tick->sell_order.end())
		{
			return sell_it->second;
		}
	}

	return 0;
}

void tick_simulator::match_entrust(const tick_info* tick)
{
	auto last_volume = _last_frame_volume.find(tick->id);
	if(last_volume == _last_frame_volume.end())
	{
		return ;
	}
	uint32_t current_volume = tick->volume - last_volume->second ;
	std::vector<order_match> match;
	_order_info.get_order_match(match,tick->id);
	for(const order_match & it : match)
	{
		handle_entrust(tick, it, current_volume);
	}
}

void tick_simulator::handle_entrust(const tick_info* tick, const order_match& match, uint32_t max_volume)
{
	order_info& order = match.order;
	if(match.state == OS_INVALID)
	{
		uint32_t err = frozen_deduction(order.est_id, order.code, order.offset, order.direction, order.last_volume, order.price);
		if (err == 0U)
		{
			this->fire_event(ET_OrderPlace, order);
			auto queue_seat = get_front_count(order.code, order.price);
			_order_info.set_seat(order.est_id, queue_seat);
			_order_info.set_state(order.est_id, OS_IN_MATCH);
		}
		else
		{
			order_error(ET_PLACE_ORDER,order.est_id, err);
		}
		return ;
	}
	if(match.state == OS_CANELED)
	{
		//撤单
		order_cancel(order);
		return;
	}

	if (order.direction == DT_LONG)
	{	
		if(order.offset == OT_OPEN)
		{
			handle_buy(tick, match, max_volume);
		}
		else if (order.offset == OT_CLOSE)
		{
			handle_sell(tick, match, max_volume);
		}
		
	}
	else if (order.direction == DT_SHORT)
	{
		if (order.offset == OT_CLOSE)
		{
			handle_buy(tick, match, max_volume);
		}
		else
		{
			handle_sell(tick, match, max_volume);
		}
	}
}
void tick_simulator::handle_sell(const tick_info* tick,const order_match& match, uint32_t max_volume)
{
	order_info& order = match.order;
	if (order.price == 0)
	{
		//市价单直接成交(暂时先不考虑一次成交不完的情况)
		order.price = _order_info.set_price(order.est_id,tick->buy_price());
	}
	if (match.flag == OF_FOK)
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
	else if (match.flag == OF_FAK)
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
				_order_info.set_seat(order.est_id,0);
				uint32_t can_deal_volume = static_cast<uint32_t>(-new_seat);
				uint32_t deal_volume = order.last_volume > can_deal_volume ? can_deal_volume : order.last_volume;
				if (deal_volume > 0U)
				{
					order_deal(order, deal_volume);
				}
			}
			else
			{
				_order_info.set_seat(order.est_id, new_seat);
			}
		}
	}

	

}

void tick_simulator::handle_buy(const tick_info* tick, const order_match& match, uint32_t max_volume)
{
	order_info& order = match.order;
	if (order.price == 0)
	{
		//市价单直接成交
		order.price = _order_info.set_price(order.est_id,tick->sell_price());
	}
	if (match.flag == OF_FOK)
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
	else if (match.flag == OF_FAK)
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
				_order_info.set_seat(order.est_id, 0);
				uint32_t can_deal_volume = static_cast<uint32_t>(-new_seat);
				uint32_t deal_volume = order.last_volume > can_deal_volume ? can_deal_volume : order.last_volume;
				if (deal_volume > 0)
				{
					order_deal(order, deal_volume);
				}
			}
			else
			{
				_order_info.set_seat(order.est_id, new_seat);
			}
		}
	}
}

void tick_simulator::order_deal(order_info& order, uint32_t deal_volume)
{
	
	auto& pos = _position_info[order.code];
	pos.id = order.code;
	if(order.offset == OT_OPEN)
	{
		//开仓
		if(order.direction == DT_LONG)
		{
			
			if(_account_info.money >= deal_volume * _service_charge)
			{
				pos.today_long.price = (pos.today_long.price * pos.today_long.postion + order.price * deal_volume) / (pos.today_long.postion + deal_volume);
				pos.today_long.postion += deal_volume;
				_account_info.money -=  deal_volume * _service_charge;
			}
			
		}
		else if (order.direction == DT_SHORT)
		{
			if(_account_info.money >= deal_volume * _service_charge)
			{
				pos.today_short.price = (pos.today_short.price * pos.today_short.postion + order.price * deal_volume) / (pos.today_short.postion + deal_volume);
				pos.today_short.postion += deal_volume;
				_account_info.money -= (deal_volume * _service_charge);
			}
		}
	}
	else
	{
		//平仓
		if (order.direction == DT_LONG)
		{
			if(pos.today_long.postion >= deal_volume&& pos.today_long.frozen >= deal_volume&& _account_info.money >= deal_volume * _service_charge&& _account_info.frozen_monery >= deal_volume * pos.today_long.price * _multiple * _margin_rate)
			{
				_account_info.money += (deal_volume * (order.price - pos.today_long.price) * _multiple);
				pos.today_long.postion -= deal_volume;
				pos.today_long.frozen -= deal_volume;
				_account_info.money -= (deal_volume * _service_charge);
				_account_info.frozen_monery -= (deal_volume * pos.today_long.price * _multiple * _margin_rate);
			}
			
		}
		else if (order.direction == DT_SHORT)
		{
			if(pos.today_short.postion >= deal_volume&& pos.today_short.frozen >= deal_volume&& _account_info.money >= (deal_volume * _service_charge)&& _account_info.frozen_monery >= std::round(deal_volume * pos.today_short.price * _multiple * _margin_rate))
			{
				_account_info.money += (deal_volume * (pos.today_short.price - order.price) * _multiple);
				pos.today_short.postion -= deal_volume;
				pos.today_short.frozen -= deal_volume;
				_account_info.money -= deal_volume * _service_charge;
				_account_info.frozen_monery -= (deal_volume * pos.today_short.price * _multiple * _margin_rate);
			}
		}
	}
	order.last_volume =_order_info.set_last_volume(order.est_id,order.last_volume - deal_volume);
	if(order.last_volume > 0)
	{
		//部分成交
		this->fire_event(ET_OrderDeal, order.est_id, order.total_volume - order.last_volume, order.total_volume);
	}
	else
	{
		//全部成交
		this->fire_event(ET_OrderTrade, order.est_id, order.code, order.offset, order.direction, order.price, order.total_volume);
		_order_info.del_order(order.est_id);
	}
	this->fire_event(ET_PositionChange, pos);
	this->fire_event(ET_AccountChange,_account_info);
}
void tick_simulator::order_error(error_type type,estid_t estid,uint32_t err)
{
	this->fire_event(ET_OrderError, type, estid, err);
	_order_info.del_order(estid);
}
void tick_simulator::order_cancel(const order_info& order)
{
	if(order.last_volume>0)
	{
		if(thawing_deduction(order.code, order.offset, order.direction, order.last_volume, order.price))
		{
			this->fire_event(ET_OrderCancel, order.est_id, order.code, order.offset, order.direction, order.price, order.last_volume, order.total_volume);
			_order_info.del_order(order.est_id);
		}
		else
		{
			LOG_ERROR("order_cancel error");
		}
	}
}
uint32_t tick_simulator::frozen_deduction(estid_t est_id,const code_t& code,offset_type offset, direction_type direction,uint32_t count,double_t price)
{
	if (offset == OT_OPEN)
	{
		double_t frozen_monery = (count * price * _multiple * _margin_rate);
		if (frozen_monery + _account_info.frozen_monery > _account_info.money)
		{
			return 31U;
		}
		//开仓 冻结保证金
		_account_info.frozen_monery += frozen_monery;
		this->fire_event(ET_AccountChange, _account_info);
		return 0U ;
	}
	if (offset == OT_CLOSE)
	{
		auto& pos = _position_info[code];
		if (direction == DT_LONG)
		{
			if(pos.today_long.postion - pos.today_long.frozen < count)
			{
				return 30U;
			}
			pos.today_long.frozen += count;
		}
		else if (direction == DT_SHORT)
		{
			if (pos.today_short.postion - pos.today_short.frozen < count)
			{
				return 30U;
			}
			pos.today_short.frozen += count;
		}
		this->fire_event(ET_PositionChange, pos);
		return 0U;
	}
	return 23U;
}
bool tick_simulator::thawing_deduction(const code_t& code, offset_type offset, direction_type direction, uint32_t last_volume, double_t price)
{

	if (offset == OT_OPEN)
	{
		double_t delta = (last_volume * price * _multiple * _margin_rate);
		//撤单 取消冻结保证金
		if(_account_info.frozen_monery < delta)
		{
			LOG_ERROR("thawing_deduction error");
			return false ;
		}
		_account_info.frozen_monery -= delta;
		this->fire_event(ET_AccountChange, _account_info);
	}
	else if (offset == OT_CLOSE)
	{
		auto& pos = _position_info[code];
		if (direction == DT_LONG)
		{
			if(pos.today_long.frozen < last_volume)
			{
				LOG_ERROR("thawing_deduction error today_long frozen < last_volume %d %d" , pos.today_long.frozen, last_volume);
				return false ;
			}
			pos.today_long.frozen -= last_volume;
		}
		else if (direction == DT_SHORT)
		{
			if(pos.today_short.frozen < last_volume)
			{
				LOG_ERROR("thawing_deduction error today_short frozen < last_volume %d %d", pos.today_long.frozen, last_volume);
				return false ;
			}
			pos.today_short.frozen -= last_volume;
		}
		this->fire_event(ET_PositionChange, pos);
	}
	return true;
}

