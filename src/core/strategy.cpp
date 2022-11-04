#include "strategy.h"
#include "pod_chain.h"
#include <log_wapper.hpp>
#include <market_api.h>
#include <trader_api.h>

strategy::strategy() :_max_position(1), _chain(nullptr), _trader(nullptr), _market(nullptr)
{
}
strategy::~strategy()
{
	if (_trader)
	{
		_trader = nullptr;
	}
	if (_market)
	{
		_market = nullptr;
	}
	if (_chain)
	{
		delete _chain;
		_chain = nullptr;
	}
}

/*
	*	³õÊ¼»¯
	*/
void strategy::init(market_api* market,trader_api* trader)
{
	_market = market;
	_trader = trader;
	_chain = new the_end_chain(trader, _max_position);
	this->on_init();
}

void strategy::entrust(const std::vector<std::any>& param)
{
	if(param.size() >= 1)
	{
		estid_t localid = std::any_cast<estid_t>(param[0]);
		this->on_entrust(localid);
	}
	
}

void strategy::deal(const std::vector<std::any>& param)
{
	if (param.size() >= 3)
	{
		estid_t localid = std::any_cast<estid_t>(param[0]);
		uint32_t deal_volume = std::any_cast<uint32_t>(param[1]);
		uint32_t total_volume = std::any_cast<uint32_t>(param[2]);
		this->on_deal(localid, deal_volume, total_volume);
	}
}

void strategy::trade(const std::vector<std::any>& param)
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

void strategy::cancel(const std::vector<std::any>& param)
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

void strategy::tick(const tick_info* tick)
{
	this->on_tick(tick);
	check_order_condition(tick);
}

estid_t strategy::buy_for_open(code_t code,uint32_t count ,double_t price , order_flag flag )
{
	if (_chain == nullptr)
	{
		LOG_ERROR("buy_for_open _chain nullptr");
		return estid_t();
	}

	return _chain->place_order(OT_OPEN, DT_LONG, code, count, price, flag);
}


estid_t strategy::sell_for_close(code_t code, uint32_t count, double_t price , order_flag flag )
{
	if (_chain == nullptr)
	{
		LOG_ERROR("sell_for_close _chain nullptr");
		return estid_t();
	}

	return _chain->place_order(OT_CLOSE, DT_LONG, code, count, price, flag);
}

estid_t strategy::sell_for_open(code_t code, uint32_t count, double_t price , order_flag flag )
{
	if (_chain == nullptr)
	{
		LOG_ERROR("sell_for_open _chain nullptr");
		return estid_t();
	}

	return _chain->place_order(OT_OPEN, DT_SHORT, code, count, price, flag);
}

estid_t strategy::buy_for_close(code_t code, uint32_t count, double_t price , order_flag flag )
{
	if (_chain == nullptr)
	{
		LOG_ERROR("buy_for_close _chain nullptr");
		return estid_t();
	}
	
	return _chain->place_order(OT_CLOSE, DT_SHORT,code, count, price, flag);
}

void strategy::cancel_order(estid_t order_id)
{
	LOG_DEBUG("cancel_order : %s\n", order_id.to_str());
	if(_trader)
	{
		_trader->cancel_order(order_id);
	}
}

void strategy::set_trading_optimize(uint32_t max_position, trading_optimal opt , bool flag )
{
	_max_position = max_position;
	if(flag)
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
			_chain = new close_to_open_chain(_trader,_max_position, new the_end_chain(_trader, _max_position));
			break;
		default:
			_chain = new the_end_chain(_trader, _max_position);
			break;
		}
	}
}

const position_info* strategy::get_position(code_t code) const
{
	if(_trader == nullptr)
	{
		LOG_ERROR("cancel_order error _trader nullptr");
		return nullptr;
	}
	return _trader->get_position(code);
}

const account_info* strategy::get_account() const
{
	if (_trader == nullptr)
	{
		LOG_ERROR("get_account error _trader nullptr");
		return nullptr;
	}
	return _trader->get_account();
}

const order_info* strategy::get_order(estid_t order_id) const
{
	if (_trader == nullptr)
	{
		LOG_ERROR("get_account error _trader nullptr");
		return nullptr;
	}
	return _trader->get_order(order_id);
}

void strategy::subscribe(const std::set<code_t>& codes) 
{
	if (_market == nullptr)
	{
		LOG_ERROR("subscribe error _market nullptr");
		return ;
	}
	_market->subscribe(codes);
}

void strategy::unsubscribe(const std::set<code_t>& codes)
{
	if (_market == nullptr)
	{
		LOG_ERROR("unsubscribe error _market nullptr");
		return ;
	}
	_market->unsubscribe(codes);
}
time_t strategy::get_last_time() const
{
	if (_market == nullptr)
	{
		LOG_ERROR("get_last_time error _market nullptr");
		return 0;
	}
	return _market->last_tick_time();
}

void strategy::set_cancel_condition(estid_t order_id,std::shared_ptr<condition> cds)
{
	LOG_DEBUG("set_timeout_cancel : %s\n", order_id.to_str());
	_need_check_condition[order_id] = cds;
}


void strategy::check_order_condition(const tick_info* tick)
{

	for(auto it = _need_check_condition.begin();it!= _need_check_condition.end();)
	{
		if(it->second->check(tick))
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


void strategy::remove_invalid_condition(estid_t order_id)
{
	auto odit = _need_check_condition.find(order_id);
	if (odit != _need_check_condition.end())
	{
		_need_check_condition.erase(odit);
	}
}
