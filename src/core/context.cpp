#include "context.h"
#include <market_api.h>
#include <trader_api.h>
#include <cpu_helper.hpp>
#include "pod_chain.h"
#include <interface.h>
#include <params.hpp>
#include <mmf_wapper.hpp>
#include <time_utils.hpp>


context::context():
	_is_runing(false),
	_realtime_thread(nullptr),
	_max_position(10000),
	_default_chain(nullptr),
	_trading_filter(nullptr),
	_is_trading_ready(false),
	_tick_callback(nullptr),
	_last_order_time(0),
	_bind_cpu_core(-1),
	_loop_interval(1),
	_ready_callback(nullptr),
	_update_callback(nullptr),
	_last_tick_time(0),
	realtime_event()
{
}
context::~context()
{
	if(_default_chain)
	{
		delete _default_chain;
		_default_chain = nullptr ;
	}
	for(auto it : _custom_chain)
	{
		delete it.second;
	}
	_custom_chain.clear();
	
}

void context::init(const params& control_config, const params& include_config,bool reset_trading_day)
{
	_max_position = control_config.get<uint32_t>("position_limit");
	_bind_cpu_core = control_config.get<int16_t>("bind_cpu_core");
	_loop_interval = control_config.get<uint32_t>("loop_interval");
	_include_config = include_config.data() ;
	_default_chain = create_chain(false);
	
	get_trader().bind_event([this](trader_event_type type, const std::vector<std::any>& param)->void {

		//LOG_INFO("event_type : ", type);
		switch (type)
		{

		case trader_event_type::TET_OrderCancel:
			handle_cancel(param);
			break;
		case trader_event_type::TET_OrderPlace:
			handle_entrust(param);
			break;
		case trader_event_type::TET_OrderDeal:
			handle_deal(param);
			break;
		case trader_event_type::TET_OrderTrade:
			handle_trade(param);
			break;
		case trader_event_type::TET_OrderError:
			handle_error(param);
			break;
		}
		});

	get_market().bind_event([this](market_event_type type, const std::vector<std::any>& param)->void {
	
		switch (type)
		{
		case market_event_type::MET_TickReceived:
			handle_tick(param);
			break;
		}
		});
}

void context::load_trader_data()
{
	LOG_INFO("context load trader data");
	auto trader_data = get_trader().get_trader_data();
	if (trader_data)
	{
		_position_info.clear();
		_order_info.clear();
		for (const auto& it : trader_data->orders)
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
			else if (it.offset == offset_type::OT_CLSTD)
			{
				if (it.direction == direction_type::DT_LONG)
				{
					pos.today_long.frozen += it.total_volume;
				}
				else if (it.direction == direction_type::DT_SHORT)
				{
					pos.today_short.frozen += it.total_volume;
				}
			}
			else
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
			_order_info[it.estid] = it;
		}
		
		for (const auto& it : trader_data->positions)
		{
			auto& pos = _position_info[it.id];
			pos.id = it.id;
			pos.today_long.postion = it.today_long ;
			pos.today_short.postion = it.today_short;
			pos.history_long.postion = it.history_long;
			pos.history_short.postion = it.history_short;
		}
	}
}

void context::start_service()
{
	_is_runing = true;
	load_trader_data();
	_realtime_thread = new std::thread([this]()->void{
		if(0 <= _bind_cpu_core && _bind_cpu_core < cpu_helper::get_cpu_cores())
		{
			if (!cpu_helper::bind_core(_bind_cpu_core))
			{
				LOG_WARNING("bind to core failed :", _bind_cpu_core);
			}
		}
		if(!_is_trading_ready)
		{
			check_crossday();
		}
		while (_is_runing||!is_terminaled())
		{
			
			auto begin = std::chrono::system_clock::now();
			this->update();
			auto use_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin);
			auto duration = std::chrono::microseconds(_loop_interval);
			if (use_time < duration)
			{
				std::this_thread::sleep_for(duration - use_time);
			}
		}
		_is_trading_ready = false;
	});
	
}

void context::update()
{
	get_market().update();
	get_trader().update();
	this->on_update();
	if (is_trading_ready())
	{
		if (this->_update_callback)
		{
			this->_update_callback();
		}
	}
	
}

void context::stop_service()
{
	while (_is_trading_ready.exchange(false));
	_is_runing = false ;
	if(_realtime_thread)
	{
		_realtime_thread->join();
		delete _realtime_thread;
		_realtime_thread = nullptr;
	}
}

pod_chain* context::create_chain(bool flag)
{

	pod_chain* chain = new verify_chain(*this);
	if (flag)
	{
		chain = new price_to_cancel_chain(*this, chain);
	}
	
	return chain;
}

pod_chain* context::get_chain(untid_t untid)
{
	auto it = _custom_chain.find(untid );
	if(it != _custom_chain.end())
	{
		return it->second;
	}
	return _default_chain;
}

order_statistic context::get_all_statistic()const
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

const tick_info& context::get_previous_tick(const code_t& code)
{
	const auto it = _previous_tick.find(code);
	if(it == _previous_tick.end())
	{
		return default_tick;
	}
	return it->second ;
}

void context::set_trading_filter(filter_callback callback)
{
	_trading_filter = callback;
}

estid_t context::place_order(untid_t untid,offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	PROFILE_DEBUG(code.get_id());
	if (!is_trading_ready())
	{
		LOG_WARNING("place order not trading ready", code.get_id());
		return INVALID_ESTID;
	}
	auto chain = get_chain(untid);
	if (chain == nullptr)
	{
		LOG_ERROR("place_order _chain nullptr");
		return INVALID_ESTID;
	}

	estid_t estid = chain->place_order(offset, direction, code, count, price, flag);
	if (estid != INVALID_ESTID)
	{
		_last_order_time = _last_tick_time;
		_statistic_info[code].place_order_amount++;
	}
	PROFILE_DEBUG(code.get_id());
	return estid ;
}

bool context::cancel_order(estid_t estid)
{
	if(estid == INVALID_ESTID)
	{
		return false;
	}
	if (!is_trading_ready())
	{
		LOG_WARNING("cancel order not trading ready ", estid);
		return false;
	}
	LOG_INFO("context cancel_order : ", estid);
	return get_trader().cancel_order(estid);
}

const position_info& context::get_position(const code_t& code)const
{
	const auto& it = _position_info.find(code);
	if (it != _position_info.end())
	{
		return (it->second);
	}
	return default_position;
}

const order_info& context::get_order(estid_t estid)const
{
	auto it = _order_info.find(estid);
	if (it != _order_info.end())
	{
		return (it->second);
	}
	return default_order;
}

void context::find_orders(std::vector<order_info>& order_result, std::function<bool(const order_info&)> func) const
{
	for (auto& it : _order_info)
	{
		if (func(it.second))
		{
			order_result.emplace_back(it.second);
		}
	}
}

uint32_t context::get_total_position() const
{
	uint32_t total = 0;
	for (const auto& it : _position_info)
	{
		total += it.second.get_total();
	}
	return total;
}
void context::subscribe(const std::set<code_t>& tick_data, tick_callback tick_cb)
{
	this->_tick_callback = tick_cb;
	get_market().subscribe(tick_data);
	
}

void context::unsubscribe(const std::set<code_t>& tick_data)
{
	get_market().unsubscribe(tick_data);
}

daytm_t context::get_last_time()
{
	return _last_tick_time;
}

daytm_t context::last_order_time()
{
	return _last_order_time;
}

const order_statistic& context::get_order_statistic(const code_t& code)const
{
	auto it = _statistic_info.find(code);
	if(it != _statistic_info.end())
	{
		return it->second;
	}
	return default_statistic;
}


uint32_t context::get_trading_day()
{
	return get_trader().get_trading_day();
}

const char* context::get_include_config(const char* key)
{
	auto it = _include_config.find(key);
	if(it == _include_config.end())
	{
		return nullptr ;
	}
	return it->second.c_str();
}

void context::use_custom_chain(untid_t untid,bool flag)
{
	auto it = _custom_chain.find(untid);
	if(it != _custom_chain.end())
	{
		delete it->second;
	}
	auto chain = create_chain(flag);
	_custom_chain[untid] = chain;
}

const today_market_info& context::get_today_market_info(const code_t& id)const
{
	auto it = _today_market_info.find(id);
	if (it == _today_market_info.end())
	{
		return default_today_market;
	}
	return it->second;
}


uint32_t context::get_total_pending()
{
	uint32_t res = 0;
	for (auto& it : _position_info)
	{
		res += (it.second.long_pending + it.second.short_pending);
	}
	return res;
}

void context::check_crossday()
{

	_is_trading_ready = true;

	if (this->_ready_callback)
	{
		this->_ready_callback();
	}
	LOG_INFO("trading ready");
}

void context::handle_entrust(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		order_info order = std::any_cast<order_info>(param[0]);
		_order_info[order.estid] = (order);
		if (order.offset != offset_type::OT_OPEN)
		{
			//平仓冻结仓位
			frozen_deduction(order.code, order.direction, order.offset,order.total_volume);
		}
		else
		{
			record_pending(order.code, order.direction, order.offset, order.total_volume);
		}
		if(realtime_event.on_entrust)
		{
			realtime_event.on_entrust(order);
		}
		_statistic_info[order.code].entrust_amount++;

	}
}

void context::handle_deal(const std::vector<std::any>& param)
{
	if (param.size() >= 3)
	{
		estid_t estid = std::any_cast<estid_t>(param[0]);
		uint32_t deal_volume = std::any_cast<uint32_t>(param[1]);
		uint32_t total_volume = std::any_cast<uint32_t>(param[2]);
		auto it = _order_info.find(estid);
		if (it != _order_info.end())
		{
			calculate_position(it->second.code, it->second.direction, it->second.offset, deal_volume, it->second.price);
			it->second.last_volume = total_volume - deal_volume;
		}
		if(realtime_event.on_deal)
		{
			realtime_event.on_deal(estid, deal_volume, total_volume);
		}
	}
}

void context::handle_trade(const std::vector<std::any>& param)
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
		if(realtime_event.on_trade)
		{
			realtime_event.on_trade(estid, code, offset, direction, price, trade_volume);
		}
		_statistic_info[code].trade_amount++;
	}
}

void context::handle_cancel(const std::vector<std::any>& param)
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
		if(realtime_event.on_cancel)
		{
			realtime_event.on_cancel(estid, code, offset, direction, price, cancel_volume, total_volume);
		}
		_statistic_info[code].cancel_amount++;
	}
}

void context::handle_tick(const std::vector<std::any>& param)
{
	
	if (param.size() >= 1)
	{
		PROFILE_DEBUG("pDepthMarketData->InstrumentID");
		tick_info&& last_tick = std::any_cast<tick_info>(param[0]);
		PROFILE_DEBUG(last_tick.id.get_id());

		_last_tick_time = last_tick.time;
		auto it = _previous_tick.find(last_tick.id);
		if(it == _previous_tick.end())
		{
			_previous_tick.insert(std::make_pair(last_tick.id, last_tick));
			return;
		}
		tick_info& prev_tick = it->second;
		auto& current_market_info = _today_market_info[last_tick.id];
		current_market_info.today_tick_info.emplace_back(last_tick);
		current_market_info.volume_distribution[last_tick.price] += static_cast<uint32_t>(last_tick.volume - prev_tick.volume);
		if (this->_tick_callback)
		{
			PROFILE_DEBUG(last_tick.id.get_id());
			this->_tick_callback(last_tick);
		}
		
		it->second = last_tick;
	}
	
}

void context::handle_error(const std::vector<std::any>& param)
{
	if (param.size() >= 3)
	{
		const error_type type = std::any_cast<error_type>(param[0]);
		const estid_t estid = std::any_cast<estid_t>(param[1]);
		const uint8_t error = std::any_cast<uint8_t>(param[2]);
		
		if(type == error_type::ET_PLACE_ORDER)
		{
			auto it = _order_info.find(estid);
			if (it != _order_info.end())
			{
				_statistic_info[it->second.code].error_amount++;
				_order_info.erase(it);
			}
		}
		if (realtime_event.on_error)
		{
			realtime_event.on_error(type, estid, static_cast<error_code>(error));
		}
	}
}


void context::calculate_position(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume, double_t price)
{
	LOG_INFO("calculate_position ", code.get_id(), dir_type, offset_type, volume, price);
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
			p.today_long.postion += volume;
			p.long_pending -= volume;

		}
		else
		{
			p.today_short.postion += volume;
			p.short_pending -= volume;
		}
	}
	else if (offset_type == offset_type::OT_CLSTD)
	{
		if (dir_type == direction_type::DT_LONG)
		{
			p.today_long.postion -= volume;
			p.today_long.frozen -= volume;
		}
		else if (dir_type == direction_type::DT_SHORT)
		{
			p.today_short.postion -= volume;
			p.today_short.frozen -= volume;
		}
	}
	else
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

void context::frozen_deduction(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume)
{
	auto it = _position_info.find(code);
	if (it == _position_info.end())
	{
		return;
	}
	position_info& pos = it->second;
	if (offset_type == offset_type::OT_CLSTD)
	{
		if (dir_type == direction_type::DT_LONG)
		{
			pos.today_long.frozen += volume;
		}
		else if (dir_type == direction_type::DT_SHORT)
		{
			pos.today_short.frozen += volume;
		}
	}
	else if (offset_type == offset_type::OT_CLOSE)
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
void context::unfreeze_deduction(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume)
{
	auto it = _position_info.find(code);
	if (it == _position_info.end())
	{
		return;
	}
	position_info& pos = it->second;
	if (offset_type == offset_type::OT_CLSTD)
	{
		if (dir_type == direction_type::DT_LONG)
		{
			if (pos.today_long.frozen > volume)
			{
				pos.today_long.frozen -= volume;
			}
			else
			{
				pos.today_long.frozen = 0;
			}
		}
		else if (dir_type == direction_type::DT_SHORT)
		{
			if (pos.today_short.frozen > volume)
			{
				pos.today_short.frozen -= volume;
			}
			else
			{
				pos.today_short.frozen = 0;
			}
		}
	}
	else if (offset_type == offset_type::OT_CLOSE)
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

void context::record_pending(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume)
{
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
}

void context::recover_pending(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume)
{
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
}