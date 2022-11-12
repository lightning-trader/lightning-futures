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
	_chain(nullptr),
	_market(nullptr),
	_trader(nullptr)
{
}
context::~context()
{

}


void context::start()
{
	_is_runing = true;
	_strategy_thread = new std::thread(std::bind(&context::run, this));
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


estid_t context::place_order(offset_type offset, direction_type direction, code_t code, uint32_t count, double_t price, order_flag flag)
{
	if (_chain == nullptr)
	{
		LOG_ERROR("buy_for_close _chain nullptr");
		return estid_t();
	}

	return _chain->place_order(OT_CLOSE, DT_SHORT, code, count, price, flag);
}

void context::cancel_order(estid_t order_id)
{
	LOG_DEBUG("cancel_order : %s\n", order_id.to_str());
	if (_trader)
	{
		_trader->cancel_order(order_id);
	}
}

void context::set_trading_optimize(uint32_t max_position, trading_optimal opt, bool flag)
{
	if (_trader == nullptr)
	{
		return ;
	}
	_max_position = max_position;
	if (flag)
	{
		switch (opt)
		{
		case TO_OPEN_TO_CLOSE:
			_chain = new price_to_cancel_chain(_trader, new open_to_close_chain(_trader, new the_end_chain(_trader, _max_position)));
			break;
		case TO_CLOSE_TO_OPEN:
			_chain = new price_to_cancel_chain(_trader, new close_to_open_chain(_trader, _max_position, new the_end_chain(_trader, _max_position)));
			break;
		default:
			_chain = new price_to_cancel_chain(_trader, new the_end_chain(_trader, _max_position));
			break;
		}
	}
	else
	{
		switch (opt)
		{
		case TO_OPEN_TO_CLOSE:
			_chain = new open_to_close_chain(_trader, new the_end_chain(_trader, _max_position));
			break;
		case TO_CLOSE_TO_OPEN:
			_chain = new close_to_open_chain(_trader, _max_position, new the_end_chain(_trader, _max_position));
			break;
		default:
			_chain = new the_end_chain(_trader, _max_position);
			break;
		}
	}
}

const position_info* context::get_position(code_t code) const
{
	if (_trader == nullptr)
	{
		LOG_ERROR("cancel_order error _trader nullptr");
		return nullptr;
	}
	return _trader->get_position(code);
}

const account_info* context::get_account() const
{
	if (_trader == nullptr)
	{
		LOG_ERROR("get_account error _trader nullptr");
		return nullptr;
	}
	return _trader->get_account();
}

const order_info* context::get_order(estid_t order_id) const
{
	if (_trader == nullptr)
	{
		LOG_ERROR("get_account error _trader nullptr");
		return nullptr;
	}
	return _trader->get_order(order_id);
}

void context::subscribe(const std::set<code_t>& codes)
{
	if (_market == nullptr)
	{
		LOG_ERROR("subscribe error _market nullptr");
		return;
	}
	_market->subscribe(codes);
}

void context::unsubscribe(const std::set<code_t>& codes)
{
	if (_market == nullptr)
	{
		LOG_ERROR("unsubscribe error _market nullptr");
		return;
	}
	_market->unsubscribe(codes);
}
time_t context::get_last_time() const
{	
	if (_market == nullptr)
	{
		LOG_ERROR("get_last_time error _market nullptr");
		return 0;
	}
	return _market->last_tick_time();
}

void context::set_cancel_condition(estid_t order_id, std::function<bool(const tick_info*)> callback)
{
	LOG_DEBUG("set_timeout_cancel : %s\n", order_id.to_str());
	_need_check_condition[order_id] = callback;
}

void context::on_update()
{

}

void context::run()
{
	int core = platform_helper::get_cpu_cores();
	if (!platform_helper::bind_core(core - 1))
	{
		LOG_ERROR("Binding to core {%d} failed", core);
	}
	if(_market == nullptr)
	{
		return ;
	}
	while (_is_runing)
	{
		std::vector<const tick_info*> result;
		_market->pop_tick_info(result);
		for (auto& it : result)
		{
			handle_tick(it);
		}
		on_update();
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}


void context::handle_event(event_type type,const std::vector<std::any>& param)
{
	
	switch(type)
	{
		case ET_AccountChange:
		break;
		case ET_BeginTrading:
			handle_begin_trading(param);
		break;
		case ET_EndTrading:
			handle_end_trading(param);
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
}

void context::handle_begin_trading(const std::vector<std::any>& param)
{
	if (_trader)
	{
		_trader->submit_settlement();
	}
}

void context::handle_end_trading(const std::vector<std::any>& param)
{
	if (_trader)
	{
		auto acc = _trader->get_account();
		LOG_INFO("ET_EndTrading %f %f", acc->money, acc->frozen_monery);
	}
	_is_runing = false;
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

void context::handle_tick(const tick_info* tick)
{
	if(tick)
	{
		this->on_tick(tick);
		check_order_condition(tick);
	}
}

void context::check_order_condition(const tick_info* tick)
{

	for (auto it = _need_check_condition.begin(); it != _need_check_condition.end();)
	{
		if (it->second(tick))
		{
			cancel_order(it->first);
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
