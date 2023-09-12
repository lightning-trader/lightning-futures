#include "context.h"
#include <market_api.h>
#include <trader_api.h>
#include <cpu_helper.hpp>
#include "pod_chain.h"
#include <interface.h>
#include <params.hpp>
#include <filesystem>
#include <mmf_wapper.hpp>
#include <time_utils.hpp>


context::context():
	_is_runing(false),
	_realtime_thread(nullptr),
	_delayed_thread(nullptr),
	_max_position(10000),
	_default_chain(nullptr),
	_trading_filter(nullptr),
	_is_trading_ready(false),
	_tick_callback(nullptr),
	_bar_callback(nullptr),
	_record_data(nullptr),
	_section_config(nullptr),
	_price_step_config(nullptr),
	_bind_cpu_core(-1),
	_loop_interval(1),
	_ready_callback(nullptr),
	_update_callback(nullptr),
	_last_tick_time(0),
	realtime_event()
{
	_record_data = maping_file<record_data>("./record_data.mmf");
}
context::~context()
{
	unmaping_file(_record_data);
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
	
	auto&& localdb_name = control_config.get<std::string>("localdb_name");
	if(!localdb_name.empty())
	{
		if(reset_trading_day)
		{
			_record_data->trading_day = 0;
		}
	}
	_max_position = control_config.get<uint32_t>("position_limit");
	_bind_cpu_core = control_config.get<int16_t>("bind_cpu_core");
	_loop_interval = control_config.get<uint32_t>("loop_interval");
	const auto& section_config = include_config.get<std::string>("section_config");
	_section_config = std::make_shared<trading_section>(section_config);
	const auto& price_step_config = include_config.get<std::string>("price_step_config");
	_price_step_config = std::make_shared<price_step>(price_step_config);
	_default_chain = create_chain(false);
	
	add_trader_handle([this](trader_event_type type, const std::vector<std::any>& param)->void {

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

	add_market_handle([this](market_event_type type, const std::vector<std::any>& param)->void {
	
		switch (type)
		{
		case market_event_type::MET_TickReceived:
			handle_tick(param);
			break;
		}
		});

	//将事件数据注册到延时分发器上去
	add_trader_handle([this](trader_event_type type, const std::vector<std::any>& param)->void {
		if(_distributor)
		{
			_distributor->fire(type, param);
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
			_order_info[it.est_id] = it;
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
		if (!is_trading_ready())
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
		clear_condition();
	});
	_delayed_thread = new std::thread([this]()->void {
		
		if (!_distributor)
		{
			return;
		}
		while (_is_runing || !_distributor->is_empty())
		{
			auto begin = std::chrono::system_clock::now();
			_distributor->update();
			auto use_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - begin);
			auto duration = std::chrono::milliseconds(_loop_interval);
			if (use_time < duration)
			{
				std::this_thread::sleep_for(duration - use_time);
			}
		}
		});
}

void context::update()
{
	this->on_update();
	if (is_trading_ready())
	{
		if (this->_update_callback)
		{
			this->_update_callback();
		}
		check_condition();
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
	if (_delayed_thread)
	{
		_delayed_thread->join();
		delete _delayed_thread;
		_delayed_thread = nullptr;
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

deal_direction context::get_deal_direction(const tick_info& prev, const tick_info& tick)
{
	if(tick.price >= prev.sell_price() || tick.price >= tick.sell_price())
	{
		return deal_direction::DD_UP;
	}
	if (tick.price <= prev.buy_price() || tick.price <= tick.buy_price())
	{
		return deal_direction::DD_DOWN;
	}
	return deal_direction::DD_FLAT;
}

void context::set_cancel_condition(estid_t order_id, condition_callback callback)
{
	LOG_DEBUG("context set_cancel_condition", order_id);
	_need_check_condition[order_id] = callback;
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
	if(!is_in_trading())
	{
		LOG_WARNING("place order code not in trading", code.get_id());
		return INVALID_ESTID;
	}

	auto chain = get_chain(untid);
	if (chain == nullptr)
	{
		LOG_ERROR("place_order _chain nullptr");
		return INVALID_ESTID;
	}

	estid_t estid = chain->place_order(offset, direction, code, count, price, flag);
	if (_record_data&& estid != INVALID_ESTID)
	{
		_record_data->last_order_time = _last_tick_time;
		_record_data->statistic_info.place_order_amount++;
	}
	PROFILE_DEBUG(code.get_id());
	return estid ;
}

void context::cancel_order(estid_t order_id)
{
	if(order_id == INVALID_ESTID)
	{
		return ;
	}
	if (!is_trading_ready())
	{
		LOG_WARNING("cancel order not trading ready ", order_id);
		return ;
	}
	if (!is_in_trading())
	{
		LOG_WARNING("cancel order not in trading ", order_id);
		return ;
	}
	LOG_INFO("context cancel_order : ", order_id);
	get_trader().cancel_order(order_id);
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

const order_info& context::get_order(estid_t order_id)const
{
	auto it = _order_info.find(order_id);
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
void context::subscribe(const std::set<code_t>& tick_data, tick_callback tick_cb, const std::map<code_t, std::set<uint32_t>>& bar_data, bar_callback bar_cb)
{
	this->_tick_callback = tick_cb;
	this->_bar_callback = bar_cb;
	get_market().subscribe(tick_data);
	for(auto& it : bar_data)
	{
		for(auto& s_it : it.second)
		{
			_bar_generator[it.first][s_it] = std::make_shared<bar_generator>(s_it,_price_step_config->get_price_step(it.first), [this, s_it](const bar_info& bar)->void {
				_today_market_info[bar.id].today_bar_info[s_it].emplace_back(bar);
				if(_bar_callback)
				{
					_bar_callback(s_it, bar);
				}
			});
		}
	}
}

void context::unsubscribe(const std::set<code_t>& tick_data, const std::map<code_t, std::set<uint32_t>>& bar_data)
{
	get_market().unsubscribe(tick_data);
	for (auto& it : bar_data)
	{
		auto s = _bar_generator.find(it.first);
		if(s != _bar_generator.end())
		{
			for (auto& s_it : it.second)
			{
				auto a = s->second.find(s_it);
				if(a != s->second.end())
				{
					s->second.erase(a);
				}
			}
		}
	}
}
daytm_t context::get_last_time()
{
	return _last_tick_time;
}

daytm_t context::last_order_time()
{
	if (_record_data)
	{
		return _record_data->last_order_time;
	}
	return -1;
}

const order_statistic& context::get_order_statistic()
{
	if(_record_data)
	{
		return _record_data->statistic_info;
	}
	return default_statistic;
}


uint32_t context::get_trading_day()
{
	return get_trader().get_trading_day();
}

daytm_t context::get_close_time()const
{
	if(_section_config == nullptr)
	{
		LOG_FATAL("section config not init");
		return 0;
	}
	return _section_config->get_close_time();
}

daytm_t context::next_open_time(daytm_t time)const
{
	if (_section_config == nullptr)
	{
		LOG_FATAL("section config not init");
		return 0;
	}
	return _section_config->next_open_time(time);
}

bool context::is_in_trading()const
{
	if (_section_config == nullptr)
	{
		LOG_FATAL("section config not init");
		return false;
	}
	return _section_config->is_in_trading(_last_tick_time);
}

bool context::is_in_trading(daytm_t time)const
{
	if (_section_config == nullptr)
	{
		LOG_FATAL("section config not init");
		return false;
	}
	return _section_config->is_in_trading(time);
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
		return default_today_market_info;
	}
	return it->second;
}


uint32_t context::get_total_pending()
{
	uint32_t res = 0;
	for (auto& it : _position_info)
	{
		res += it.second.long_pending + it.second.short_pending;
	}
	return res;
}

void context::check_crossday()
{
	if (_record_data)
	{
		uint32_t trading_day = get_trader().get_trading_day();
		if (trading_day != _record_data->trading_day)
		{
			LOG_INFO("cross day %d", trading_day);
			_record_data->statistic_info.place_order_amount = 0;
			_record_data->statistic_info.entrust_amount = 0;
			_record_data->statistic_info.trade_amount = 0;
			_record_data->statistic_info.cancel_amount = 0;
			_record_data->statistic_info.error_amount = 0;
			_record_data->trading_day = trading_day;
			_today_market_info.clear();
		}

		while (_is_trading_ready.exchange(true));
		
		if (this->_ready_callback)
		{
			this->_ready_callback();
		}
		LOG_INFO("trading ready");
	}
	else
	{
		LOG_ERROR("record data mapping error");
	}

}

void context::handle_entrust(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		order_info order = std::any_cast<order_info>(param[0]);
		_order_info[order.est_id] = (order);
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
		if(_record_data)
		{
			_record_data->statistic_info.entrust_amount++;
		}
	}
}

void context::handle_deal(const std::vector<std::any>& param)
{
	if (param.size() >= 3)
	{
		estid_t localid = std::any_cast<estid_t>(param[0]);
		uint32_t deal_volume = std::any_cast<uint32_t>(param[1]);
		uint32_t total_volume = std::any_cast<uint32_t>(param[2]);
		auto it = _order_info.find(localid);
		if (it != _order_info.end())
		{
			calculate_position(it->second.code, it->second.direction, it->second.offset, deal_volume, it->second.price);
			it->second.last_volume = total_volume - deal_volume;
		}
		if(realtime_event.on_deal)
		{
			realtime_event.on_deal(localid, deal_volume, total_volume);
		}
	}
}

void context::handle_trade(const std::vector<std::any>& param)
{
	if (param.size() >= 6)
	{

		estid_t localid = std::any_cast<estid_t>(param[0]);
		code_t code = std::any_cast<code_t>(param[1]);
		offset_type offset = std::any_cast<offset_type>(param[2]);
		direction_type direction = std::any_cast<direction_type>(param[3]);
		double_t price = std::any_cast<double_t>(param[4]);
		uint32_t trade_volume = std::any_cast<uint32_t>(param[5]);
		auto it = _order_info.find(localid);
		if (it != _order_info.end())
		{
			_order_info.erase(it);
		}
		if(realtime_event.on_trade)
		{
			realtime_event.on_trade(localid, code, offset, direction, price, trade_volume);
		}
		remove_condition(localid);
		if (_record_data)
		{
			_record_data->statistic_info.trade_amount++;
		}
	}
}

void context::handle_cancel(const std::vector<std::any>& param)
{
	if (param.size() >= 7)
	{
		estid_t localid = std::any_cast<estid_t>(param[0]);
		code_t code = std::any_cast<code_t>(param[1]);
		offset_type offset = std::any_cast<offset_type>(param[2]);
		direction_type direction = std::any_cast<direction_type>(param[3]);
		double_t price = std::any_cast<double_t>(param[4]);
		uint32_t cancel_volume = std::any_cast<uint32_t>(param[5]);
		uint32_t total_volume = std::any_cast<uint32_t>(param[6]);
		auto it = _order_info.find(localid);
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
			realtime_event.on_cancel(localid, code, offset, direction, price, cancel_volume, total_volume);
		}
		remove_condition(localid);
		if (_record_data)
		{
			_record_data->statistic_info.cancel_amount++;
		}
	}
}

void context::handle_tick(const std::vector<std::any>& param)
{
	
	if (param.size() >= 1)
	{
		PROFILE_DEBUG("pDepthMarketData->InstrumentID");
		tick_info&& last_tick = std::any_cast<tick_info>(param[0]);
		PROFILE_DEBUG(last_tick.id.get_id());

		if(!_section_config->is_in_trading(last_tick.time))
		{
			LOG_WARNING("not in trading", last_tick.time);
			return;
		}
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
			deal_info deal_info;
			deal_info.volume_delta = static_cast<uint32_t>(last_tick.volume - prev_tick.volume);
			deal_info.interest_delta = last_tick.open_interest - prev_tick.open_interest;
			deal_info.direction = get_deal_direction(prev_tick, last_tick);
			PROFILE_DEBUG(last_tick.id.get_id());
			this->_tick_callback(last_tick, deal_info);
		}
		auto tk_it = _bar_generator.find(last_tick.id);
		if (tk_it != _bar_generator.end())
		{
			for (auto bg_it : tk_it->second)
			{
				bg_it.second->insert_tick(last_tick);
			}
		}
		it->second = last_tick;
	}
	
}

void context::handle_error(const std::vector<std::any>& param)
{
	if (param.size() >= 2)
	{
		const error_type type = std::any_cast<error_type>(param[0]);
		const estid_t localid = std::any_cast<estid_t>(param[1]);
		const uint32_t error_code = std::any_cast<uint32_t>(param[2]);
		if (_record_data)
		{
			_record_data->statistic_info.error_amount++;
		}
		if(type == error_type::ET_PLACE_ORDER)
		{
			auto it = _order_info.find(localid);
			if (it != _order_info.end())
			{
				_order_info.erase(it);
			}
		}
		if (realtime_event.on_error)
		{
			realtime_event.on_error(type, localid, error_code);
		}
	}
}

void context::check_condition()
{

	for (auto it = _need_check_condition.begin(); it != _need_check_condition.end();)
	{
		if (it->second(it->first))
		{
			get_trader().cancel_order(it->first);
			it = _need_check_condition.erase(it);
		}
		else
		{
			++it;
		}
	}
}


void context::remove_condition(estid_t order_id)
{
	auto odit = _need_check_condition.find(order_id);
	if (odit != _need_check_condition.end())
	{
		_need_check_condition.erase(odit);
	}
}

void context::clear_condition()
{
	_need_check_condition.clear();
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
			if (p.today_long.postion > volume)
			{
				p.today_long.postion -= volume;
			}
			else
			{
				p.today_long.postion = 0;
			}
			if (p.today_long.frozen > volume)
			{
				p.today_long.frozen -= volume;
			}
			else
			{
				p.today_long.frozen = 0;
			}
		}
		else if (dir_type == direction_type::DT_SHORT)
		{
			if (p.today_short.postion > volume)
			{
				p.today_short.postion -= volume;
			}
			else
			{
				p.today_short.postion = 0;
			}
			if (p.today_short.frozen > volume)
			{
				p.today_short.frozen -= volume;
			}
			else
			{
				p.today_short.frozen = 0;
			}
		}
	}
	else
	{
		if (dir_type == direction_type::DT_LONG)
		{
			if (p.history_long.postion > volume)
			{
				p.history_long.postion -= volume;
			}
			else
			{
				p.history_long.postion = 0;
			}
			if (p.history_long.frozen > volume)
			{
				p.history_long.frozen -= volume;
			}
			else
			{
				p.history_long.frozen = 0;
			}
		}
		else if (dir_type == direction_type::DT_SHORT)
		{
			if (p.history_short.postion > volume)
			{
				p.history_short.postion -= volume;
			}
			else
			{
				p.history_short.postion = 0;
			}
			if (p.history_short.frozen > volume)
			{
				p.history_short.frozen -= volume;
			}
			else
			{
				p.history_short.frozen = 0;
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
		auto& pos = _position_info[code];
		if (dir_type == direction_type::DT_LONG)
		{
			pos.long_pending -= volume;
		}
		else if (dir_type == direction_type::DT_SHORT)
		{
			pos.short_pending -= volume;
		}
	}
}