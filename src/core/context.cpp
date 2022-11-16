#include "context.h"
#include <file_wapper.hpp>
#include <market_api.h>
#include <trader_api.h>
#include <recorder.h>
#include <platform_helper.hpp>
#include <log_wapper.hpp>
#include "pod_chain.h"


context::context():
	_is_runing(false),
	_strategy_thread(nullptr),
	_max_position(1),
	_chain(nullptr)
{
	
}
context::~context()
{
	if(_chain)
	{
		delete _chain ;
		_chain = nullptr ;
	}
}

bool context::init()
{
	auto trader = get_trader();
	if(trader == nullptr)
	{
		return false ;
	}
	_chain = new the_end_chain(trader, _max_position);
	this->add_handle([this](event_type type, const std::vector<std::any>& param)->void {
		switch (type)
		{
		case ET_AccountChange:
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



estid_t context::place_order(offset_type offset, direction_type direction, code_t code, uint32_t count, double_t price, order_flag flag)
{
	if (_chain == nullptr)
	{
		LOG_ERROR("buy_for_close _chain nullptr");
		return INVALID_ESTID;
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

const position_info* context::get_position(code_t code)
{
	auto trader = get_trader();
	if (trader == nullptr)
	{
		LOG_ERROR("cancel_order error _trader nullptr");
		return nullptr;
	}
	static position_info cache_data ;
	cache_data = trader->get_position(code);
	return &cache_data;
}

const account_info* context::get_account()
{
	auto trader = get_trader();
	if (trader == nullptr)
	{
		LOG_ERROR("get_account error _trader nullptr");
		return nullptr;
	}
	static account_info cache_data;
	cache_data = trader->get_account();
	return &cache_data;
}

const order_info* context::get_order(estid_t order_id)
{
	auto trader = get_trader();
	if (trader == nullptr)
	{
		LOG_ERROR("get_account error _trader nullptr");
		return nullptr;
	}
	static order_info cache_data;
	cache_data = trader->get_order(order_id);
	return &cache_data;
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

void context::handle_begin(const std::vector<std::any>& param)
{
	auto trader = get_trader();
	if(trader)
	{
		LOG_INFO("ET_BeginTrading submit_settlement\n");
		trader->submit_settlement();
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
}

void context::handle_entrust(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		estid_t localid = std::any_cast<estid_t>(param[0]);
		this->on_entrust(localid);
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
