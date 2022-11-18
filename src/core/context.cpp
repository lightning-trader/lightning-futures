#include "context.h"
#include <file_wapper.hpp>
#include <market_api.h>
#include <trader_api.h>
#include <recorder.h>
#include <platform_helper.hpp>
#include <log_wapper.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include "pod_chain.h"
#include <interface.h>


context::context():
	_is_runing(false),
	_strategy_thread(nullptr),
	_max_position(1),
	_chain(nullptr),
	_trading_filter(nullptr),
	_userdata_block(10),
	_userdata_size(1024),
	_recorder(nullptr),
	_is_trading(false)
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
	if(_chain)
	{
		delete _chain ;
		_chain = nullptr ;
	}
	if(_recorder)
	{
		destory_recorder(_recorder);
	}
}

bool context::init(boost::property_tree::ptree& localdb, boost::property_tree::ptree& rcd_config)
{
	auto& localdb_name = localdb.get<std::string>("localdb_name");
	if(!localdb_name.empty())
	{
		size_t operational_size = localdb.get<size_t>("operational_size",1024);
		_userdata_block = localdb.get<size_t>("userdata_block", 10);
		_userdata_size = localdb.get<size_t>("userdata_size", 1024);
		load_data(localdb_name.c_str(), operational_size);
	}

	_recorder = create_recorder(rcd_config);

	auto trader = get_trader();
	if (trader == nullptr)
	{
		return false;
	}
	_chain = new the_end_chain(trader, _max_position);
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
		case ET_BeginTrading:
			handle_begin(param);
			break;
		case ET_EndTrading:
			handle_end(param);
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
		}
		});
	return true ;
}

void context::start()
{
	_is_runing = true;
	_strategy_thread = new std::thread([this]()->void{
		int core = platform_helper::get_cpu_cores();
		if (!platform_helper::bind_core(core - 1))
		{
			LOG_ERROR("Binding to core {%d} failed", core);
		}
		while (_is_runing)
		{
			this->update();
			//LOG_DEBUG("context update \n");
			//std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	});
	_strategy_thread->detach();
}

void context::stop()
{
	_is_runing = false ;
	if(_strategy_thread)
	{
		_strategy_thread->join();
		delete _strategy_thread;
		_strategy_thread = nullptr;
	}
}



void context::set_trading_optimize(uint32_t max_position, trading_optimal opt, bool flag)
{

	auto trader = get_trader();
	if(trader == nullptr)
	{
		return ;
	}
	_max_position = max_position;
	if (_chain)
	{
		delete _chain;
		_chain = nullptr;
	}
	if (flag)
	{
		switch (opt)
		{
		case TO_OPEN_TO_CLOSE:
			_chain = new price_to_cancel_chain(trader, new open_to_close_chain(trader, new the_end_chain(trader, _max_position)));
			break;
		case TO_CLOSE_TO_OPEN:
			_chain = new price_to_cancel_chain(trader, new close_to_open_chain(trader, _max_position, new the_end_chain(trader, _max_position)));
			break;
		default:
			_chain = new price_to_cancel_chain(trader, new the_end_chain(trader, _max_position));
			break;
		}
	}
	else
	{
		switch (opt)
		{
		case TO_OPEN_TO_CLOSE:
			_chain = new open_to_close_chain(trader, new the_end_chain(trader, _max_position));
			break;
		case TO_CLOSE_TO_OPEN:
			_chain = new close_to_open_chain(trader, _max_position, new the_end_chain(trader, _max_position));
			break;
		default:
			_chain = new the_end_chain(trader, _max_position);
			break;
		}
	}
}

void context::set_cancel_condition(estid_t order_id, condition_callback callback)
{
	LOG_DEBUG("set_timeout_cancel : %llu\n", order_id);
	_need_check_condition[order_id] = callback;
}

void context::set_trading_filter(filter_callback callback)
{
	_trading_filter = callback;
}

estid_t context::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	if (_chain == nullptr)
	{
		LOG_ERROR("place_order _chain nullptr");
		return INVALID_ESTID;
	}
	if(_trading_filter&&!_trading_filter())
	{
		LOG_DEBUG("place_order trading filter false");
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
	return _chain->place_order(offset, direction, code, count, price, flag);
}

void context::cancel_order(estid_t order_id)
{
	if(order_id == INVALID_ESTID)
	{
		return ;
	}
	LOG_DEBUG("cancel_order : %llu\n", order_id);
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
	if(index < 0||index >= _userdata_block)
	{
		return nullptr;
	}
	return _userdata_region[index]->get_address();
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
	if (_recorder)
	{
		const auto& account = get_account();
		_recorder->record_account_flow(get_last_time(), account);
	}
}

void context::handle_position(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		auto code = std::any_cast<code_t>(param[0]);
		if (_recorder)
		{
			const auto& position = get_position(code);
			_recorder->record_position_flow(get_last_time(), code, position);
		}
	}
	
}

void context::handle_crossday(const std::vector<std::any>& param)
{
	auto trader = get_trader();
	if (trader)
	{
		LOG_INFO("ET_CrossDay submit_settlement\n");
		trader->submit_settlement();
	}
}


void context::handle_begin(const std::vector<std::any>& param)
{
	_is_trading = true ;
	
	if (param.size() >= 1)
	{
		uint32_t trading_day = std::any_cast<uint32_t>(param[0]);
		LOG_INFO("ET_BeginTrading %u\n", trading_day);
	}
	if(_operational_data)
	{
		_operational_data->clear();
	}
}

void context::handle_end(const std::vector<std::any>& param)
{
	auto trader = get_trader();
	if(trader)
	{
		auto acc = trader->get_account();
		LOG_INFO("ET_EndTrading %f %f\n", acc.money, acc.frozen_monery);
	}
	_is_trading = false;
}

void context::handle_entrust(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		estid_t localid = std::any_cast<estid_t>(param[0]);
		this->on_entrust(localid);
		if(_operational_data)
		{
			_operational_data->statistic_info.cancel_amount++;
		}
		if (_recorder)
		{
			const auto& order = get_order(localid);
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
		this->on_deal(localid, deal_volume, total_volume);
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
		this->on_trade(localid, code, offset, direction, price, trade_volume);
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
		this->on_cancel(localid, code, offset, direction, price, cancel_volume, total_volume);
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
		const tick_info* tick = std::any_cast<const tick_info*>(param[0]);
		if (tick)
		{
			this->on_tick(tick);
			check_order_condition(tick);
		}
	}
	
}

void context::check_order_condition(const tick_info* tick)
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
