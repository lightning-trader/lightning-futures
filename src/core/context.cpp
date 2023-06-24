#include "context.h"
#include <market_api.h>
#include <trader_api.h>
#include <recorder.h>
#include <cpu_helper.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include "pod_chain.h"
#include <interface.h>


context::context():
	_is_runing(false),
	_realtime_thread(nullptr),
	_delayed_thread(nullptr),
	_max_position(10000),
	_default_chain(nullptr),
	_trading_filter(nullptr),
	_userdata_size(1024 * MAX_UNITID),
	_is_trading_ready(false),
	_tick_callback(nullptr),
	_bar_callback(nullptr),
	_record_region(nullptr),
	_record_data(nullptr),
	_section(nullptr),
	_fast_mode(false),
	_loop_interval(1),
	_ready_callback(nullptr),
	_update_callback(nullptr),
	_last_tick_time(0),
	realtime_event()
{

}
context::~context()
{
	if(_record_region)
	{
		_record_region->flush();
	}
	for(auto it : _userdata_region)
	{
		it->flush();
	}
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

void context::init(boost::property_tree::ptree& ctrl, boost::property_tree::ptree& include_config,
	bool reset_trading_day)
{
	auto&& localdb_name = ctrl.get<std::string>("localdb_name");
	if(!localdb_name.empty())
	{
		_userdata_size = ctrl.get<size_t>("userdata_size", 4096 * MAX_UNITID);
		load_data(localdb_name.c_str());
		if(reset_trading_day)
		{
			_record_data->trading_day = 0;
		}
	}
	_max_position = ctrl.get<uint32_t>("position_limit", 10000);
	_fast_mode = ctrl.get<bool>("fast_mode", false);
	_loop_interval = ctrl.get<uint32_t>("loop_interval", 1);
	const auto& section_config = include_config.get<std::string>("section_config", "./section.csv");
	_section = std::make_shared<trading_section>(section_config);
	_default_chain = create_chain(false);
	auto trader_data = get_trader().get_trader_data();
	if (trader_data)
	{
		_account_info = trader_data->account;
		_order_info.clear();
		for (const auto& it : trader_data->orders)
		{
			_order_info[it.est_id] = it;
		}
		_position_info.clear();
		for (const auto& it : trader_data->positions)
		{
			_position_info[it.id] = it;
		}
	}

	add_trader_handle([this](trader_event_type type, const std::vector<std::any>& param)->void {

		LOG_INFO("event_type %d\n", type);
		switch (type)
		{
		case trader_event_type::TET_SettlementCompleted:
			handle_settlement(param);
			break;
		case trader_event_type::TET_AccountChange:
			handle_account(param);
			break;
		case trader_event_type::TET_PositionChange:
			handle_position(param);
			break;
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

void context::start_service()
{
	_is_trading_ready = false;
	_is_runing = true;
	_realtime_thread = new std::thread([this]()->void{
		if(_fast_mode)
		{
			int core = cpu_helper::get_cpu_cores();
			if (!cpu_helper::bind_core(core - 1))
			{
				LOG_ERROR("Binding to core {%d} failed", core);
			}
		}
		while (_is_runing||!is_terminaled())
		{

			auto begin = std::chrono::system_clock::now();
			this->update();
			if (!_fast_mode)
			{
				auto use_time =  std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin);
				auto duration = std::chrono::microseconds(_loop_interval);
				if(use_time < duration)
				{
					std::this_thread::sleep_for(duration - use_time);
				}
			}
		}
	});
	_delayed_thread = new std::thread([this]()->void {
		
		if (!_distributor)
		{
			return;
		}
		while (_is_runing || !_distributor->is_empty())
		{
			auto begin = std::chrono::system_clock::now();
			auto use_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - begin);
			auto duration = std::chrono::milliseconds(_loop_interval);
			_distributor->update();
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
	}
}

void context::stop_service()
{
	_is_trading_ready = false ;
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
	LOG_DEBUG("context set_cancel_condition : %llu\n", order_id);
	_need_check_condition[order_id] = callback;
}

void context::set_trading_filter(filter_callback callback)
{
	_trading_filter = callback;
}

estid_t context::place_order(untid_t untid,offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	
	if (!_is_trading_ready)
	{
		LOG_DEBUG("place_order _is_trading_ready");
		return INVALID_ESTID;
	}
	if(!_section->is_in_trading(get_last_time()))
	{
		LOG_DEBUG("place_order code not in trading %s", code.get_id());
		return INVALID_ESTID;
	}

	auto chain = get_chain(untid);
	if (chain == nullptr)
	{
		LOG_ERROR("place_order _chain nullptr");
		return INVALID_ESTID;
	}
	if(_record_data)
	{
		_record_data->last_order_time = _last_tick_time;
		_record_data->statistic_info.place_order_amount++;
	}
	return chain->place_order(offset, direction, code, count, price, flag);
}

void context::cancel_order(estid_t order_id)
{
	if(order_id == INVALID_ESTID)
	{
		return ;
	}
	if (!_is_trading_ready)
	{
		return ;
	}
	if (!_section->is_in_trading(get_last_time()))
	{
		LOG_DEBUG("cancel_order code in trading %lld", order_id);
		return ;
	}
	LOG_DEBUG("context cancel_order : %llu\n", order_id);
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

const account_info& context::get_account()const
{
	return (_account_info);
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
			_bar_generator[it.first][s_it] = std::make_shared<bar_generator>(s_it,[this, s_it](const bar_info& bar)->void{
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
time_t context::get_last_time()
{
	return _last_tick_time;
}

time_t context::last_order_time()
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

void* context::get_userdata(untid_t index,size_t size)
{
	if(size > _userdata_size)
	{
		return nullptr;
	}
	return _userdata_region[index]->get_address();
}

uint32_t context::get_trading_day()
{
	return get_trader().get_trading_day();
}

time_t context::get_close_time()
{
	if(_section == nullptr)
	{
		return 0;
	}
	return _section->get_clase_time();
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


uint32_t context::get_pending_position(const code_t& code, offset_type offset, direction_type direction)
{
	std::vector<order_info> order_list;
	find_orders(order_list, [&code,offset, direction](const order_info& order)->bool {
		return order.code==code && order.offset == offset && order.direction == direction;
		});
	uint32_t res = 0;
	for (auto& it : order_list)
	{
		res += it.last_volume;
	}
	return res;
}

uint32_t context::get_open_pending()
{
	std::vector<order_info> order_list;
	find_orders(order_list, [](const order_info& order)->bool {
		return order.offset == offset_type::OT_OPEN;
		});
	uint32_t res = 0;
	for (auto& it : order_list)
	{
		res += it.last_volume;
	}
	return res;
}

void context::load_data(const char* localdb_name)
{
	std::string record_dbname = std::string("record_db:") + localdb_name;
	boost::interprocess::shared_memory_object record_shdmem
	{
		boost::interprocess::open_or_create,
		record_dbname.c_str(),
		boost::interprocess::read_write
	};
	record_shdmem.truncate(sizeof(record_data));
	_record_region = std::make_shared<boost::interprocess::mapped_region>(record_shdmem, boost::interprocess::read_write);
	_record_data = static_cast<record_data*>(_record_region->get_address());

	//用户数据
	std::string uesrdb_name = std::string("uesr_db:") + localdb_name;
	boost::interprocess::shared_memory_object userdb_shdmem
	{
		boost::interprocess::open_or_create, 
		uesrdb_name.c_str(),
		boost::interprocess::read_write 
	};
	userdb_shdmem.truncate(_userdata_size * MAX_UNITID);
	for(size_t i=0;i < MAX_UNITID;i++)
	{
		boost::interprocess::offset_t offset = i * _userdata_size;
		auto region = std::make_shared<boost::interprocess::mapped_region>(userdb_shdmem, boost::interprocess::read_write, offset, _userdata_size);
		_userdata_region.emplace_back(region);
	}
}

void context::check_crossday(uint32_t trading_day)
{
	if (_record_data)
	{
		if (trading_day != _record_data->trading_day)
		{
			LOG_INFO("cross day %d", trading_day);
			
			
			_record_data->statistic_info.place_order_amount = 0;
			_record_data->statistic_info.entrust_amount = 0;
			_record_data->statistic_info.trade_amount = 0;
			_record_data->statistic_info.cancel_amount = 0;
			_record_data->statistic_info.error_amount = 0;
			_record_data->trading_day = trading_day;
			LOG_INFO("submit_settlement");
			get_trader().submit_settlement();
		}
		else
		{
			if (!_is_trading_ready)
			{
				_is_trading_ready = true;
			}

			if (this->_ready_callback)
			{
				this->_ready_callback();
			}
			LOG_INFO("trading ready");
		}

	}

}

void context::handle_settlement(const std::vector<std::any>& param)
{
	if (!_is_trading_ready)
	{
		_is_trading_ready = true ;
	}
	
	if(this->_ready_callback)
	{
		this->_ready_callback();
	}
	LOG_INFO("trading ready");
	
}

void context::handle_account(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		const auto& account = std::any_cast<account_info>(param[0]);
		_account_info = account;
	}

}

void context::handle_position(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		auto position = std::any_cast<position_info>(param[0]);
		_position_info[position.id] = position;
	}
}

void context::handle_entrust(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		order_info order = std::any_cast<order_info>(param[0]);
		_order_info[order.est_id] = (order);
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
		remove_invalid_condition(localid);
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
			_order_info.erase(it);
		}
		if(realtime_event.on_cancel)
		{
			realtime_event.on_cancel(localid, code, offset, direction, price, cancel_volume, total_volume);
		}
		remove_invalid_condition(localid);
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
		tick_info&& last_tick = std::any_cast<tick_info>(param[0]);
		if (!is_trading_ready())
		{
			uint32_t trading_day = get_trader().get_trading_day();
			_section->init(trading_day, last_tick.time);
			check_crossday(trading_day);
		}
		if(!_section->is_in_trading(last_tick.time))
		{
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
		
		if (last_tick.trading_day != _record_data->trading_day)
		{
			_today_market_info.clear();
		}
		auto& current_market_info = _today_market_info[last_tick.id];
		current_market_info.today_tick_info.emplace_back(last_tick);
		current_market_info.volume_distribution[last_tick.price] += (last_tick.volume - prev_tick.volume);
		if (this->_tick_callback)
		{
			deal_info deal_info;
			deal_info.volume_delta = last_tick.volume - prev_tick.volume;
			deal_info.interest_delta = last_tick.open_interest - prev_tick.open_interest;
			deal_info.direction = get_deal_direction(prev_tick, last_tick);
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
		
		check_order_condition(last_tick);
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

void context::check_order_condition(const tick_info& tick)
{

	for (auto it = _need_check_condition.begin(); it != _need_check_condition.end();)
	{
		if (it->second(it->first,tick))
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


void context::remove_invalid_condition(estid_t order_id)
{
	auto odit = _need_check_condition.find(order_id);
	if (odit != _need_check_condition.end())
	{
		_need_check_condition.erase(odit);
	}
}
