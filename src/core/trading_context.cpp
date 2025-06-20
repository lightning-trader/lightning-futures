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
#include "trading_context.h"
#include <trader_api.h>
#include <interface.h>
#include <params.hpp>
#include <time_utils.hpp>
#include <process_helper.hpp>
#include "time_section.h"
#include <log_define.hpp>


using namespace lt;

trading_context::trading_context(market_api* market, trader_api* trader,const char* section_file, bool is_simlator,state_listener* state_listener):
	_trader(trader),
	_last_order_time(0),
	_market(market),
	_tick_callback(nullptr),
	_last_tick_time(0),
	_trading_section(std::make_unique<time_section>(section_file)),
	_state_listener(state_listener),
	_tick_update_point(std::chrono::system_clock::now())
{
	if(is_simlator)
	{
		_last_tick_time = _trading_section->get_open_time();
	}
	else
	{	
		_last_tick_time = lt::make_daytm(time(0),0U);
	}
	if (_market)
	{
		_market->bind_event(market_event_type::MET_TickReceived, std::bind(&trading_context::handle_tick, this, std::placeholders::_1));
	}
	if (_trader)
	{
		_trader->bind_event(trader_event_type::TET_OrderCancel, std::bind(&trading_context::handle_cancel, this, std::placeholders::_1));
		_trader->bind_event(trader_event_type::TET_OrderPlace, std::bind(&trading_context::handle_entrust, this, std::placeholders::_1));
		_trader->bind_event(trader_event_type::TET_OrderDeal, std::bind(&trading_context::handle_deal, this, std::placeholders::_1));
		_trader->bind_event(trader_event_type::TET_OrderTrade, std::bind(&trading_context::handle_trade, this, std::placeholders::_1));
		_trader->bind_event(trader_event_type::TET_OrderError, std::bind(&trading_context::handle_error, this, std::placeholders::_1));
		_trader->bind_event(trader_event_type::TET_StateChange, std::bind(&trading_context::handle_state, this, std::placeholders::_1));
	}
}
trading_context::~trading_context()
{
	if (_market)
	{
		_market->clear_event();
	}
	if (_trader)
	{
		_trader->clear_event();
	}
}


bool trading_context::load_data()
{
	if(!_trader)
	{
		LOG_ERROR("context load trader null");
		return false;
	}
	LOG_INFO("context load trader data");
	
	const auto orders = _trader->get_all_orders();
	_order_info.clear();
	for (const auto& it : orders)
	{
		auto& pos = _position_info[it.code];
		pos.id = it.code;
		if (it.offset == offset_type::OT_OPEN)
		{
			if (it.direction == direction_type::DT_LONG)
			{
				pos.long_pending += it.total_volume;
			}
			else if (it.direction == direction_type::DT_SHORT)
			{
				pos.short_pending += it.total_volume;
			}
		}
		
		else
		{
			if (it.direction == direction_type::DT_LONG)
			{
				pos.total_long.frozen += it.total_volume;
			}
			else if (it.direction == direction_type::DT_SHORT)
			{
				pos.total_short.frozen += it.total_volume;
			}
			if(it.offset == offset_type::OT_CLOSE)
			{
				if (it.direction == direction_type::DT_LONG)
				{
					pos.history_long.frozen += it.total_volume;
				}
				else if (it.direction == direction_type::DT_SHORT)
				{
					pos.history_short.frozen += it.total_volume;
				}
			}
			

		}
		_order_info[it.estid] = it;
	}

	const auto positions = _trader->get_all_positions();
	_position_info.clear();
	for (const auto& it : positions)
	{
		auto& pos = _position_info[it.id];
		pos.id = it.id;
		pos.total_long.postion = it.total_long;
		pos.total_short.postion = it.total_short;
		pos.history_long.postion = it.history_long;
		pos.history_short.postion = it.history_short;
	}
	auto instruments = _trader->get_all_instruments();
	_instrument_info.clear();
	for(auto it : instruments)
	{
		_instrument_info.insert(std::make_pair(it.code, it));
	}
	return true;
}



void trading_context::update()
{
	if (_market)
	{
		_market->update();
	}
	if(is_trading_time())
	{
		if (_trader)
		{
			_trader->update();
		}

		this->check_condition();
	}
}


order_statistic trading_context::get_all_statistic()const
{
	order_statistic all_statistic;
	for (const auto& it : _statistic_info)
	{
		 all_statistic.place_order_amount += it.second.place_order_amount;
		 all_statistic.entrust_amount += it.second.entrust_amount;
		 all_statistic.trade_amount += it.second.trade_amount;
		 all_statistic.cancel_amount += it.second.cancel_amount;
		 all_statistic.error_amount += it.second.error_amount;
	}
	return all_statistic;
}

void trading_context::set_cancel_condition(estid_t estid, std::function<bool(estid_t)> callback, std::function<void(estid_t)> error)
{
	if (estid != INVALID_ESTID)
	{
		LOG_DEBUG("set_cancel_condition : ", estid);
		_need_check_condition[estid] = std::make_pair(callback,error);
	}
}


estid_t trading_context::place_order(order_listener* listener, offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	LOG_INFO("context place order : ", code.get_symbol(), offset, direction, price, count);
	PROFILE_DEBUG(code.get_symbol());
	if (!this->_trader)
	{
		LOG_ERROR("place order this->_trader null");
		return INVALID_ESTID;
	}
	if (!is_trading_time())
	{
		LOG_WARNING("place order not in trading time", code.get_symbol());
		return INVALID_ESTID;
	}
	PROFILE_DEBUG(code.get_symbol());
	auto& contract = this->get_instrument(code);
	double_t real_price = std::round(price / contract.price_step) * contract.price_step;
	if (_filter_function)
	{
		if (!_filter_function(code, offset, direction, count, real_price, flag))
		{
			LOG_WARNING("engine place order : _filter_function false", code.get_symbol(), offset, direction, real_price, count);
			return INVALID_ESTID;
		}
	}

	estid_t estid = this->_trader->place_order(offset, direction, code, count, real_price, flag);
	if (estid != INVALID_ESTID)
	{
		_order_listener[estid] = listener;
		_statistic_info[code].place_order_amount++;
	}
	PROFILE_DEBUG(code.get_symbol());
	return estid ;
}

bool trading_context::cancel_order(estid_t estid)
{

	if(estid == INVALID_ESTID)
	{
		return false;
	}
	if (!this->_trader)
	{
		LOG_ERROR("cancel order this->_trader null");
		return false;
	}
	if (!is_trading_time())
	{
		LOG_WARNING("cancel order not in trading time ", estid);
		return false;
	}
	if(_cancel_freeze.find(estid) != _cancel_freeze.end())
	{
		LOG_WARNING("cancel order in freeze ", estid);
		return true;
	}
	
	LOG_INFO("context cancel_order : ", estid);
	auto result = this->_trader->cancel_order(estid);
	if(result)
	{
		_cancel_freeze.insert(estid);
	}
	return result;
}

const position_info& trading_context::get_position(const code_t& code)const
{
	const auto& it = _position_info.find(code);
	if (it != _position_info.end())
	{
		return (it->second);
	}
	return default_position;
}

const order_info& trading_context::get_order(estid_t estid)const
{
	auto it = _order_info.find(estid);
	if (it != _order_info.end())
	{
		return (it->second);
	}
	return default_order;
}

void trading_context::find_orders(std::vector<order_info>& order_result, std::function<bool(const order_info&)> func) const
{
	for (auto& it : _order_info)
	{
		if (func(it.second))
		{
			order_result.emplace_back(it.second);
		}
	}
}

uint32_t trading_context::get_total_position() const
{
	uint32_t total = 0;
	for (const auto& it : _position_info)
	{
		total += it.second.get_total();
	}
	return total;
}

daytm_t trading_context::last_order_time()const
{
	return _last_order_time;
}

const order_statistic& trading_context::get_order_statistic(const code_t& code)const
{
	auto it = _statistic_info.find(code);
	if(it != _statistic_info.end())
	{
		return it->second;
	}
	return default_statistic;
}


uint32_t trading_context::get_trading_day()const
{
	if(!this->_trader)
	{
		return 0X0U;
	}
	return this->_trader->get_trading_day();
}

bool trading_context::is_trading_time()const
{
	return _trading_section->is_trading_time(get_last_time());
}


uint32_t trading_context::get_total_pending()
{
	uint32_t res = 0;
	for (auto& it : _position_info)
	{
		res += (it.second.long_pending + it.second.short_pending);
	}
	return res;
}

void trading_context::check_crossday()
{
	_statistic_info.clear();
	_last_order_time = 0U;
	LOG_INFO("trading ready");
}
const tick_info& trading_context::get_previous_tick(const code_t& code)const
{
	const auto it = _previous_tick.find(code);
	if (it == _previous_tick.end())
	{
		return default_tick;
	}
	return it->second;
}

void trading_context::subscribe(const std::set<code_t>& tick_data, std::function<void(const tick_info&)> tick_callback)
{
	_previous_tick.clear();
	_market_info.clear();
	this->_tick_callback = tick_callback;
	if (this->_market)
	{
		this->_market->subscribe(tick_data);
	}
}

void trading_context::unsubscribe(const std::set<code_t>& tick_data)
{
	if (this->_market)
	{
		this->_market->unsubscribe(tick_data);
	}
}


daytm_t trading_context::get_last_time()const
{
	std::chrono::time_point now = std::chrono::system_clock::now();
	std::chrono::duration<double, std::milli> elapsed = now - _tick_update_point;
	return _last_tick_time + static_cast<daytm_t>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}

seqtm_t trading_context::get_now_time()const
{
	return lt::make_seqtm(get_trading_day(),get_last_time());
}

const market_info& trading_context::get_market_info(const code_t& id)const
{
	auto it = _market_info.find(id);
	if (it == _market_info.end())
	{
		return default_market;
	}
	return it->second;
}
const tick_info& trading_context::get_last_tick(const code_t& id)const
{
	auto last_it = _market_info.find(id);
	if (last_it != _market_info.end())
	{
		return last_it->second.last_tick_info;
	}
	const auto it = _previous_tick.find(id);
	if (it != _previous_tick.end())
	{
		return it->second;
	}
	return default_tick;
}


void trading_context::update_time(daytm_t time)
{
	_last_tick_time = time;
	_tick_update_point = std::chrono::system_clock::now();
}

void trading_context::handle_tick(const std::vector<std::any>& param)
{

	if (param.size() >= 2)
	{
		PROFILE_DEBUG("pDepthMarketData->InstrumentID");
		auto&& last_tick = std::any_cast<const tick_info>(param[0]);
		PROFILE_DEBUG(last_tick.id.get_symbol());
		LOG_INFO("handle_tick", last_tick.id.get_symbol(), last_tick.time, _last_tick_time, last_tick.price);
		if (last_tick.time > _last_tick_time)
		{
			update_time(last_tick.time);
		}

		auto it = _previous_tick.find(last_tick.id);
		if (it != _previous_tick.end())
		{
			tick_info& prev_tick = it->second;
			if (_trading_section->is_trading_time(last_tick.time))
			{
				auto&& extend_data = std::any_cast<tick_extend>(param[1]);
				auto& current_market_info = _market_info[last_tick.id];
				current_market_info.code = last_tick.id;
				current_market_info.last_tick_info = last_tick;
				current_market_info.open_price = std::get<TEI_OPEN_PRICE>(extend_data);
				current_market_info.close_price = std::get<TEI_CLOSE_PRICE>(extend_data);
				current_market_info.standard_price = std::get<TEI_STANDARD_PRICE>(extend_data);
				current_market_info.high_price = std::get<TEI_HIGH_PRICE>(extend_data);
				current_market_info.low_price = std::get<TEI_LOW_PRICE>(extend_data);
				current_market_info.max_price = std::get<TEI_MAX_PRICE>(extend_data);
				current_market_info.min_price = std::get<TEI_MIN_PRICE>(extend_data);
				current_market_info.trading_day = get_trading_day();
				current_market_info.volume_distribution[last_tick.price] += static_cast<uint32_t>(last_tick.volume - prev_tick.volume);
				if (this->_tick_callback)
				{
					PROFILE_DEBUG(last_tick.id.get_symbol());
					this->_tick_callback(last_tick);
				}
			}
			it->second = last_tick;
		}
		else
		{
			_previous_tick.insert(std::make_pair(last_tick.id, last_tick));
		}
	}
}

void trading_context::handle_entrust(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		order_info order = std::any_cast<order_info>(param[0]);
		_order_info[order.estid] = (order);
		if (order.offset == offset_type::OT_OPEN)
		{
			record_pending(order.code, order.direction, order.offset, order.total_volume);
		}
		else
		{
			//平仓冻结仓位
			frozen_deduction(order.code, order.direction, order.offset, order.total_volume);
		}
		auto it = _order_listener.find(order.estid);
		if(it != _order_listener.end() && it->second)
		{
			it->second->on_entrust(order);
		}
		_last_order_time = order.create_time;
		_statistic_info[order.code].entrust_amount++;

	}
}

void trading_context::handle_deal(const std::vector<std::any>& param)
{
	if (param.size() >= 3)
	{
		estid_t estid = std::any_cast<estid_t>(param[0]);
		uint32_t deal_volume = std::any_cast<uint32_t>(param[1]);
		uint32_t last_volume = std::any_cast<uint32_t>(param[2]);
		auto it = _order_info.find(estid);
		if (it != _order_info.end())
		{
			calculate_position(it->second.code, it->second.direction, it->second.offset, deal_volume, it->second.price);
			it->second.last_volume = last_volume;
		}
		auto listener_iter = _order_listener.find(estid);
		if (listener_iter != _order_listener.end() && listener_iter->second)
		{
			listener_iter->second->on_deal(estid, deal_volume);
		}
	}
}

void trading_context::handle_trade(const std::vector<std::any>& param)
{
	if (param.size() >= 6)
	{

		estid_t estid = std::any_cast<estid_t>(param[0]);
		code_t code = std::any_cast<code_t>(param[1]);
		offset_type offset = std::any_cast<offset_type>(param[2]);
		direction_type direction = std::any_cast<direction_type>(param[3]);
		double_t price = std::any_cast<double_t>(param[4]);
		uint32_t trade_volume = std::any_cast<uint32_t>(param[5]);
		auto it = _order_info.find(estid);
		if (it != _order_info.end())
		{
			_order_info.erase(it);
		}
		auto listener_iter = _order_listener.find(estid);
		if (listener_iter != _order_listener.end() && listener_iter->second)
		{
			listener_iter->second->on_trade(estid, code, offset, direction, price, trade_volume);
			_order_listener.erase(listener_iter);
		}
		auto odit = _need_check_condition.find(estid);
		if (odit != _need_check_condition.end())
		{
			_need_check_condition.erase(odit);
		}
		_statistic_info[code].trade_amount++;
	}
}

void trading_context::handle_cancel(const std::vector<std::any>& param)
{
	if (param.size() >= 7)
	{
		estid_t estid = std::any_cast<estid_t>(param[0]);
		code_t code = std::any_cast<code_t>(param[1]);
		offset_type offset = std::any_cast<offset_type>(param[2]);
		direction_type direction = std::any_cast<direction_type>(param[3]);
		double_t price = std::any_cast<double_t>(param[4]);
		uint32_t cancel_volume = std::any_cast<uint32_t>(param[5]);
		uint32_t total_volume = std::any_cast<uint32_t>(param[6]);
		auto it = _order_info.find(estid);
		if(it != _order_info.end())
		{
			//撤销解冻仓位
			if (offset == offset_type::OT_OPEN)
			{
				recover_pending(code, direction, offset, cancel_volume);
			}
			else
			{
				unfreeze_deduction(code, direction, offset, cancel_volume);
			}
			_order_info.erase(it);
		}
		auto listener_iter = _order_listener.find(estid);
		if (listener_iter != _order_listener.end() && listener_iter->second)
		{
			listener_iter->second->on_cancel(estid, code, offset, direction, price, cancel_volume, total_volume);
			_order_listener.erase(listener_iter);
		}
		auto odit = _need_check_condition.find(estid);
		if (odit != _need_check_condition.end())
		{
			_need_check_condition.erase(odit);
		}
		auto cfit = _cancel_freeze.find(estid);
		if (cfit != _cancel_freeze.end())
		{
			_cancel_freeze.erase(cfit);
		}
		_statistic_info[code].cancel_amount++;
	}
}

void trading_context::handle_error(const std::vector<std::any>& param)
{
	if (param.size() >= 3)
	{
		const error_type type = std::any_cast<error_type>(param[0]);
		const estid_t estid = std::any_cast<estid_t>(param[1]);
		const uint8_t error = std::any_cast<uint8_t>(param[2]);
		
		auto it = _order_info.find(estid);
		if (it != _order_info.end())
		{
			_statistic_info[it->second.code].error_amount++;
			if (type == error_type::ET_PLACE_ORDER)
			{
				_order_info.erase(it);
			}
		}
		auto listener_iter = _order_listener.find(estid);
		if (listener_iter != _order_listener.end() && listener_iter->second)
		{
			listener_iter->second->on_error(type, estid, static_cast<error_code>(error));
			if (type == error_type::ET_PLACE_ORDER)
			{
				_order_listener.erase(listener_iter);
			}
		}
		if (type == error_type::ET_PLACE_ORDER)
		{
			auto odit = _need_check_condition.find(estid);
			if (odit != _need_check_condition.end())
			{
				_need_check_condition.erase(odit);
			}
		}
		else if (type == error_type::ET_CANCEL_ORDER)
		{
			auto it = _cancel_freeze.find(estid);
			if(it != _cancel_freeze.end())
			{
				_cancel_freeze.erase(it);
			}
		}
		else
		{
			LOG_ERROR("handle_error", error);
		}
	}
}

void trading_context::handle_state(const std::vector<std::any>& param)
{

	if (param.size() < 3)
	{
		return;
	}
	const code_t& product_code = std::any_cast<std::string>(param[0]);
	const bool current_state = std::any_cast<bool>(param[1]);
	const daytm_t time = std::any_cast<daytm_t>(param[2]);
	if (_state_listener)
	{
		if (current_state)
		{
			_state_listener->on_resume(product_code);
		}
		else
		{
			_state_listener->on_pause(product_code);
		}

	}
	LOG_INFO("state change : ", product_code.to_string(), current_state, time);
}

void trading_context::calculate_position(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume, double_t price)
{
	LOG_INFO("calculate_position ", code.get_symbol(), dir_type, offset_type, volume, price);
	position_info p;
	auto it = _position_info.find(code);
	if (it != _position_info.end())
	{
		p = it->second;
	}
	else
	{
		p.id = code;
	}
	if (offset_type == offset_type::OT_OPEN)
	{
		if (dir_type == direction_type::DT_LONG)
		{
			p.total_long.postion += volume;
			p.long_pending -= volume;

		}
		else
		{
			p.total_short.postion += volume;
			p.short_pending -= volume;
		}
	}
	
	else
	{
		if (dir_type == direction_type::DT_LONG)
		{
			p.total_long.postion -= volume;
			p.total_long.frozen -= volume;
		}
		else if (dir_type == direction_type::DT_SHORT)
		{
			p.total_short.postion -= volume;
			p.total_short.frozen -= volume;
		}
		
		if (offset_type == offset_type::OT_CLOSE)
		{
			if (dir_type == direction_type::DT_LONG)
			{
				p.history_long.postion -= volume;
				p.history_long.frozen -= volume;
			}
			else if (dir_type == direction_type::DT_SHORT)
			{
				p.history_short.postion -= volume;
				p.history_short.frozen -= volume;
			}
		}
	}
	if (!p.empty())
	{
		_position_info[code] = p;
	}
	else
	{
		if (it != _position_info.end())
		{
			_position_info.erase(it);
		}
	}
	print_position("calculate_position");
}

void trading_context::frozen_deduction(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume)
{
	auto it = _position_info.find(code);
	if (it == _position_info.end())
	{
		return;
	}
	position_info& pos = it->second;
	if (dir_type == direction_type::DT_LONG)
	{
		pos.total_long.frozen += volume;
	}
	else if (dir_type == direction_type::DT_SHORT)
	{
		pos.total_short.frozen += volume;
	}
	if (offset_type == offset_type::OT_CLOSE)
	{
		if (dir_type == direction_type::DT_LONG)
		{
			pos.history_long.frozen += volume;
		}
		else if (dir_type == direction_type::DT_SHORT)
		{
			pos.history_short.frozen += volume;
		}
	}
	print_position("frozen_deduction");
}
void trading_context::unfreeze_deduction(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume)
{
	auto it = _position_info.find(code);
	if (it == _position_info.end())
	{
		return;
	}
	position_info& pos = it->second;
	if (dir_type == direction_type::DT_LONG)
	{
		if (pos.total_long.frozen > volume)
		{
			pos.total_long.frozen -= volume;
		}
		else
		{
			pos.total_long.frozen = 0;
		}
	}
	else if (dir_type == direction_type::DT_SHORT)
	{
		if (pos.total_short.frozen > volume)
		{
			pos.total_short.frozen -= volume;
		}
		else
		{
			pos.total_short.frozen = 0;
		}
	}
	if (offset_type == offset_type::OT_CLOSE)
	{
		if (dir_type == direction_type::DT_LONG)
		{
			if (pos.history_long.frozen > volume)
			{
				pos.history_long.frozen -= volume;
			}
			else
			{
				pos.history_long.frozen = 0;
			}
		}
		else if (dir_type == direction_type::DT_SHORT)
		{
			if (pos.history_short.frozen > volume)
			{
				pos.history_short.frozen -= volume;
			}
			else
			{
				pos.history_short.frozen = 0;
			}
		}
	}
	print_position("thawing_deduction");
}

void trading_context::record_pending(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume)
{
	print_position("record_pending begin");
	if(offset_type== offset_type::OT_OPEN)
	{
		auto& pos = _position_info[code];
		pos.id = code ;
		if(dir_type == direction_type::DT_LONG)
		{
			pos.long_pending += volume;
		}
		else if (dir_type == direction_type::DT_SHORT)
		{
			pos.short_pending += volume;
		}
	}
	print_position("record_pending end");
}

void trading_context::recover_pending(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume)
{
	print_position("recover_pending begin");
	if (offset_type == offset_type::OT_OPEN)
	{
		auto it = _position_info.find(code);
		if(it != _position_info.end())
		{
			if (dir_type == direction_type::DT_LONG)
			{
				it->second.long_pending -= volume;
			}
			else if (dir_type == direction_type::DT_SHORT)
			{
				it->second.short_pending -= volume;
			}
		}
	}
	print_position("recover_pending end");
}


void trading_context::set_trading_filter(filter_function callback)
{
	_filter_function = callback;
}

void trading_context::check_condition()
{
	std::map<estid_t, std::function<void(estid_t)>> error_condition;
	for (auto it = _need_check_condition.begin(); it != _need_check_condition.end();)
	{
		if (it->second.first(it->first))
		{
			if (!this->cancel_order(it->first))
			{
				error_condition.insert(std::make_pair(it->first, it->second.second));
			}	
			it = _need_check_condition.erase(it);
		}
		else
		{
			++it;
		}
	}
	for (const auto& it: error_condition)
	{
		if (it.second)
		{
			it.second(it.first);
		}
	}
}

void trading_context::remove_condition(estid_t estid)
{
	auto odit = _need_check_condition.find(estid);
	if (odit != _need_check_condition.end())
	{
		_need_check_condition.erase(odit);
	}
}

void trading_context::clear_condition()
{
	_need_check_condition.clear();
}

const instrument_info& trading_context::get_instrument(const code_t& code)const
{
	auto it = _instrument_info.find(code);
	if (it != _instrument_info.end())
	{
		return it->second;
	}
	return default_instrument;
}

void trading_context::regist_order_listener(estid_t estid, order_listener* listener)
{
	_order_listener[estid] = listener;
}

void trading_context::print_position(const char* title)
{
	if (!_position_info.empty())
	{
		LOG_INFO("print_position : ", title);
	}
	for (const auto& it : _position_info)
	{
		const auto& pos = it.second;
		LOG_INFO("position :", pos.id.get_symbol(), "total_long(", pos.total_long.postion, pos.total_long.frozen, ") total_short(", pos.total_short.postion, pos.total_short.frozen, ") history_long(", pos.history_long.postion, pos.history_long.frozen, ") history_short(", pos.history_short.postion, pos.history_short.frozen, ")");
		LOG_INFO("pending :", pos.id.get_symbol(), pos.long_pending, pos.short_pending);
	}
}