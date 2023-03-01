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
	_userdata_block(16),
	_userdata_size(1024),
	_recorder(nullptr),
	_is_trading_ready(false),
	on_tick(nullptr),
	on_entrust(nullptr),
	on_deal(nullptr),
	on_trade(nullptr),
	on_cancel(nullptr),
	on_error(nullptr),
	on_ready(nullptr),
	_operational_region(nullptr),
	_operational_data(nullptr),
	_section(nullptr),
	_fast_mode(false),
	_loop_interval(1)
{

}
context::~context()
{
	if(_operational_region)
	{
		_operational_region->flush();
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
		size_t operational_size = ctrl.get<size_t>("operational_size",1024);
		_userdata_block = ctrl.get<size_t>("userdata_block", 16);
		_userdata_size = ctrl.get<size_t>("userdata_size", 1024);
		load_data(localdb_name.c_str(), operational_size);
	}
	_max_position = ctrl.get<uint16_t>("position_limit", 10000);
	_fast_mode = ctrl.get<bool>("fast_mode", false);
	_loop_interval = ctrl.get<uint32_t>("loop_interval", 1);

	_recorder = create_recorder(rcd_config);

	auto trader = get_trader();
	if (trader == nullptr)
	{
		return false;
	}

	const auto& section_config = include_config.get<std::string>("section_config", "./section.csv");
	_section = std::make_shared<trading_section>(section_config);
	

	_default_chain = create_chain(TO_INVALID, false, [this](const code_t& code, offset_type offset, direction_type direction, order_flag flag)->bool{
		if(_trading_filter==nullptr)
		{
			return true ;
		}
		return _trading_filter(code, offset, direction, flag);
	});
	this->add_handle([this](event_type type, const std::vector<std::any>& param)->void {
		switch (type)
		{
		case ET_AccountChange:
			handle_account(param);
			break;
		case ET_PositionChange:
			handle_position(param);
			break;
		case ET_CrossDay:
			handle_crossday(param);
			break;
		case ET_TradingReady:
			handle_ready(param);
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



pod_chain* context::create_chain(trading_optimal opt, bool flag, std::function<bool(const code_t& code, offset_type offset, direction_type direction, order_flag flag)> fliter_callback)
{

	auto trader = get_trader();
	if(trader == nullptr)
	{
		return nullptr;
	}
	pod_chain* chain = nullptr ;
	if (flag)
	{
		switch (opt)
		{
		case TO_OPEN_TO_CLOSE:
			chain = new price_to_cancel_chain(trader, new open_to_close_chain(trader, new verify_chain(trader, _max_position, fliter_callback)));
			break;
		case TO_CLOSE_TO_OPEN:
			chain = new price_to_cancel_chain(trader, new close_to_open_chain(trader, _max_position, new verify_chain(trader, _max_position, fliter_callback)));
			break;
		default:
			chain = new price_to_cancel_chain(trader, new verify_chain(trader, _max_position, fliter_callback));
			break;
		}
	}
	else
	{
		switch (opt)
		{
		case TO_OPEN_TO_CLOSE:
			chain = new open_to_close_chain(trader, new verify_chain(trader, _max_position, fliter_callback));
			break;
		case TO_CLOSE_TO_OPEN:
			chain = new close_to_open_chain(trader, _max_position, new verify_chain(trader, _max_position, fliter_callback));
			break;
		default:
			chain = new verify_chain(trader, _max_position, fliter_callback);
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
	if(_operational_data)
	{
		auto market = get_market();
		if(market)
		{
			_operational_data->last_order_time = market->last_tick_time();
		}
		_operational_data->statistic_info.place_order_amount++;
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
	auto trader = get_trader();
	if (trader)
	{
		trader->cancel_order(order_id);
	}
}

const position_info& context::get_position(const code_t& code)
{
	auto trader = get_trader();
	if (trader == nullptr)
	{
		LOG_ERROR("cancel_order error _trader nullptr");
		return default_position;
	}
	return trader->get_position(code);
}

const account_info& context::get_account()
{
	auto trader = get_trader();
	if (trader == nullptr)
	{
		LOG_ERROR("get_account error _trader nullptr");
		return default_account;
	}
	return trader->get_account();
}

const order_info& context::get_order(estid_t order_id)
{
	auto trader = get_trader();
	if (trader == nullptr)
	{
		LOG_ERROR("get_account error _trader nullptr");
		return default_order;
	}
	return trader->get_order(order_id);
}

void context::subscribe(const std::set<code_t>& codes)
{
	auto market = get_market();
	if (market == nullptr)
	{
		LOG_ERROR("subscribe error _market nullptr");
		return;
	}
	market->subscribe(codes);
}

void context::unsubscribe(const std::set<code_t>& codes)
{
	auto market = get_market();
	if (market == nullptr)
	{
		LOG_ERROR("unsubscribe error _market nullptr");
		return;
	}
	market->unsubscribe(codes);
}
time_t context::get_last_time()
{
	auto market = get_market();
	if (market == nullptr)
	{
		LOG_ERROR("get_last_time error _market nullptr");
		return 0;
	}
	return market->last_tick_time();
}

time_t context::last_order_time()
{
	if (_operational_data)
	{
		return _operational_data->last_order_time;
	}
	return -1;
}

const order_statistic& context::get_order_statistic()
{
	if(_operational_data)
	{
		return _operational_data->statistic_info;
	}
	return default_statistic;
}

void* context::get_userdata(uint32_t index,size_t size)
{
	if(size > _userdata_size)
	{
		return nullptr;
	}
	if(index >= _userdata_block)
	{
		return nullptr;
	}
	return _userdata_region[index]->get_address();
}

uint32_t context::get_trading_day()
{
	const auto market = get_market();
	if(market == nullptr)
	{
		return 0U;
	}
	return market->get_trading_day();
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
	auto chain = create_chain(opt, flag, [this](const code_t& code, offset_type offset, direction_type direction, order_flag flag)->bool {
		if (_trading_filter == nullptr)
		{
			return true;
		}
		return _trading_filter(code, offset, direction, flag);
		});
	_custom_chain[untid] = chain;
}

void context::load_data(const char* localdb_name,size_t oper_size)
{
	boost::interprocess::shared_memory_object shdmem
	{
		boost::interprocess::open_or_create, 
		localdb_name,
		boost::interprocess::read_write 
	};
	shdmem.truncate(oper_size + _userdata_block * _userdata_size);
	_operational_region = std::make_shared<boost::interprocess::mapped_region>(shdmem,boost::interprocess::read_write);
	_operational_data = static_cast<operational_data*>(_operational_region->get_address());

	for(size_t i=0;i < _userdata_block;i++)
	{
		boost::interprocess::offset_t offset = oper_size + i * _userdata_size;
		auto region = std::make_shared<boost::interprocess::mapped_region>(shdmem, boost::interprocess::read_write, offset, _userdata_size);
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
	if (param.size() >= 1)
	{
		uint32_t trading_day = std::any_cast<uint32_t>(param[0]);
		LOG_INFO("cross day %d", trading_day);
	}
	auto trader = get_trader();
	if (trader)
	{
		LOG_INFO("submit_settlement");
		trader->submit_settlement();
	}
	if (_operational_data)
	{
		_operational_data->clear();
	}
}


void context::handle_ready(const std::vector<std::any>& param)
{
	if (!_is_trading_ready)
	{
		_is_trading_ready = true ;
	}
	_section->init(get_trading_day(),get_last_time());
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
		if(_operational_data)
		{
			_operational_data->statistic_info.entrust_amount++;
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
		if (_operational_data)
		{
			_operational_data->statistic_info.trade_amount++;
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
		if (_operational_data)
		{
			_operational_data->statistic_info.cancel_amount++;
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
		if(this->on_tick)
		{
			this->on_tick(last_tick);
		}
		check_order_condition(last_tick);
	}
	
}

void context::handle_error(const std::vector<std::any>& param)
{
	if (param.size() >= 2)
	{
		const error_type type = std::any_cast<error_type>(param[0]);
		const estid_t localid = std::any_cast<estid_t>(param[1]);
		const uint32_t error_code = std::any_cast<uint32_t>(param[2]);
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
			auto trader = get_trader();
			if(trader)
			{
				trader->cancel_order(it->first);
			}
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
