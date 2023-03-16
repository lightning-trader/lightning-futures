#include "context.h"
#include <file_wapper.hpp>
#include <market_api.h>
#include <trader_api.h>
#include <recorder.h>
#include <cpu_helper.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include "pod_chain.h"
#include <interface.h>


context::context():
	_is_runing(false),
	_strategy_thread(nullptr),
	_max_position(10000),
	_default_chain(nullptr),
	_trading_filter(nullptr),
	_userdata_size(1024 * MAX_UNITID),
	_recorder(nullptr),
	_is_trading_ready(false),
	on_tick(nullptr),
	on_entrust(nullptr),
	on_deal(nullptr),
	on_trade(nullptr),
	on_cancel(nullptr),
	on_error(nullptr),
	on_ready(nullptr),
	_record_region(nullptr),
	_record_data(nullptr),
	_section(nullptr),
	_fast_mode(false),
	_loop_interval(1)
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
	if(_recorder)
	{
		destory_recorder(_recorder);
	}
}

bool context::init(boost::property_tree::ptree& ctrl, boost::property_tree::ptree& include_config, boost::property_tree::ptree& rcd_config)
{
	auto& localdb_name = ctrl.get<std::string>("localdb_name");
	if(!localdb_name.empty())
	{
		_userdata_size = ctrl.get<size_t>("userdata_size", 4096 * MAX_UNITID);
		load_data(localdb_name.c_str());
	}
	_max_position = ctrl.get<uint16_t>("position_limit", 10000);
	_fast_mode = ctrl.get<bool>("fast_mode", false);
	_loop_interval = ctrl.get<uint32_t>("loop_interval", 1);

	_recorder = create_recorder(rcd_config);

	const auto& section_config = include_config.get<std::string>("section_config", "./section.csv");
	_section = std::make_shared<trading_section>(section_config);
	

	_default_chain = create_chain(TO_INVALID, false);
	this->add_handle([this](event_type type, const std::vector<std::any>& param)->void {
		switch (type)
		{
		case ET_AccountChange:
			handle_account(param);
			break;
		case ET_PositionChange:
			handle_position(param);
			break;
		case ET_FirstMessage:
			handle_crossday(param);
			break;
		case ET_SettlementCompleted:
			handle_settlement(param);
			break;
		case ET_TickReceived:
			handle_tick(param);
			break;
		case ET_OrderCancel:
			handle_cancel(param);
			break;
		case ET_OrderPlace:
			handle_entrust(param);
			break;
		case ET_OrderDeal:
			handle_deal(param);
			break;
		case ET_OrderTrade:
			handle_trade(param);
			break;
		case ET_OrderError:
			handle_error(param);
			break;
		}
		});
	return true ;
}

void context::start_service()
{
	_is_runing = true;
	_strategy_thread = new std::thread([this]()->void{
		if(_fast_mode)
		{
			int core = cpu_helper::get_cpu_cores();
			if (!cpu_helper::bind_core(core - 1))
			{
				LOG_ERROR("Binding to core {%d} failed", core);
			}
		}
		while (_is_runing)
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
	//_strategy_thread->detach();
}

void context::stop_service()
{
	_is_runing = false ;
	if(_strategy_thread)
	{
		_strategy_thread->join();
		delete _strategy_thread;
		_strategy_thread = nullptr;
	}
}



pod_chain* context::create_chain(trading_optimal opt, bool flag)
{

	pod_chain* chain = new verify_chain(*this);
	if (flag)
	{
		switch (opt)
		{
		case TO_OPEN_TO_CLOSE:
			chain = new price_to_cancel_chain(*this, new open_to_close_chain(*this, chain));
			break;
		case TO_CLOSE_TO_OPEN:
			chain = new price_to_cancel_chain(*this, new close_to_open_chain(*this, chain));
			break;
		default:
			chain = new price_to_cancel_chain(*this, chain);
			break;
		}
	}
	else
	{
		switch (opt)
		{
		case TO_OPEN_TO_CLOSE:
			chain = new open_to_close_chain(*this, chain);
			break;
		case TO_CLOSE_TO_OPEN:
			chain = new close_to_open_chain(*this, chain);
			break;
		default:
			break;
		}
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
		return DD_UP;
	}
	if (tick.price <= prev.buy_price() || tick.price <= tick.buy_price())
	{
		return DD_DOWN;
	}
	return DD_FLAT;
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
		LOG_DEBUG("place_order code in trading %s", code.get_id());
		return INVALID_ESTID;
	}

	auto chain = get_chain(untid);
	if (chain == nullptr)
	{
		LOG_ERROR("place_order _chain nullptr");
		return INVALID_ESTID;
	}
	if(_record_region)
	{
		_record_data->last_order_time = get_market().last_tick_time();
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

const position_info& context::get_position(const code_t& code)
{
	return get_trader().get_position(code);
}

const account_info& context::get_account()
{
	return get_trader().get_account();
}

const order_info& context::get_order(estid_t order_id)
{
	return get_trader().get_order(order_id);
}

void context::subscribe(const std::set<code_t>& codes)
{
	get_market().subscribe(codes);
}

void context::unsubscribe(const std::set<code_t>& codes)
{
	get_market().unsubscribe(codes);
}
time_t context::get_last_time()
{
	return get_market().last_tick_time();
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
	return get_market().get_trading_day();
}

time_t context::get_close_time()
{
	if(_section == nullptr)
	{
		return 0;
	}
	return _section->get_clase_time();
}

void context::use_custom_chain(untid_t untid,trading_optimal opt, bool flag)
{
	auto it = _custom_chain.find(untid);
	if(it != _custom_chain.end())
	{
		delete it->second;
	}
	auto chain = create_chain(opt, flag);
	_custom_chain[untid] = chain;
}


void context::load_data(const char* localdb_name)
{
	std::string record_dbname = std::string("record_db") + localdb_name;
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
	std::string uesrdb_name = std::string("uesr_db") + localdb_name;
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

void context::handle_account(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		const auto& account = std::any_cast<account_info>(param[0]);
		if (_recorder)
		{
			_recorder->record_account_flow(get_last_time(), account);
		}
	}
	
}

void context::handle_position(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		auto position = std::any_cast<position_info>(param[0]);
		if (_recorder)
		{
			_recorder->record_position_flow(get_last_time(), position);
		}
	}
	
}

void context::handle_crossday(const std::vector<std::any>& param)
{
	if (param.size() < 1)
	{
		return ;
	}
	uint32_t trading_day = std::any_cast<uint32_t>(param[0]);
	LOG_INFO("cross day %d", trading_day);
	_section->init(trading_day, get_last_time());
	if (trading_day != _record_data->trading_day)
	{
		if (_record_data)
		{
			//记录结算数据
			if (_recorder && trading_day > _record_data->trading_day)
			{
				_recorder->record_crossday_flow(get_last_time(), _record_data->trading_day, _record_data->statistic_info, get_trader().get_account());
			}
			
			_record_data->statistic_info.place_order_amount = 0;
			_record_data->statistic_info.entrust_amount = 0;
			_record_data->statistic_info.trade_amount = 0;
			_record_data->statistic_info.cancel_amount = 0;
			_record_data->statistic_info.error_amount = 0;
			_record_data->trading_day = trading_day;
		}

		LOG_INFO("submit_settlement");
		get_trader().submit_settlement();
	}
	else
	{
		 if (!_is_trading_ready)
		 {
			 _is_trading_ready = true;
		 }

		 if (on_ready)
		 {
			 on_ready();
		 }
		 LOG_INFO("trading ready");
	}
}


void context::handle_settlement(const std::vector<std::any>& param)
{
	if (!_is_trading_ready)
	{
		_is_trading_ready = true ;
	}
	
	if(on_ready)
	{
		on_ready();
	}
	LOG_INFO("trading ready");
	
}


void context::handle_entrust(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		order_info order = std::any_cast<order_info>(param[0]);
		if(this->on_entrust)
		{
			this->on_entrust(order);
		}
		if(_record_data)
		{
			_record_data->statistic_info.entrust_amount++;
		}
		if (_recorder)
		{
			LOG_TRACE("context handle_entrust %lld %s", order.est_id, order.code.get_id());
			_recorder->record_order_entrust(get_last_time(), order);
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
		if(this->on_deal)
		{
			this->on_deal(localid, deal_volume, total_volume);
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
		if(this->on_trade)
		{
			this->on_trade(localid, code, offset, direction, price, trade_volume);
		}
		remove_invalid_condition(localid);
		if (_record_data)
		{
			_record_data->statistic_info.trade_amount++;
		}
		if (_recorder)
		{
			_recorder->record_order_trade(get_last_time(), localid);
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
		if(this->on_cancel)
		{
			this->on_cancel(localid, code, offset, direction, price, cancel_volume, total_volume);
		}
		remove_invalid_condition(localid);
		if (_record_data)
		{
			_record_data->statistic_info.cancel_amount++;
		}
		if (_recorder)
		{
			_recorder->record_order_cancel(get_last_time(), localid, cancel_volume);
		}
	}
}

void context::handle_tick(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		tick_info last_tick = std::any_cast<tick_info>(param[0]);
		auto it = _previous_tick.find(last_tick.id);
		if(it == _previous_tick.end())
		{
			_previous_tick.insert(std::make_pair(last_tick.id, last_tick));
			return;
		}
		tick_info& prev_tick = it->second;
		if(this->on_tick)
		{	
			deal_info deal_info;
			deal_info.volume_delta = last_tick.volume - prev_tick.volume;
			deal_info.interest_delta = last_tick.open_interest - prev_tick.open_interest;
			deal_info.direction = get_deal_direction(prev_tick, last_tick);
			this->on_tick(last_tick, deal_info);
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
		if (this->on_error)
		{
			this->on_error(type, localid, error_code);
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
