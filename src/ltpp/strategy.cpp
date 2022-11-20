#include "strategy.h"
#include "lightning.h"
#include "strategy_manager.h"

strategy::strategy()
{
}
strategy::~strategy()
{
	
}

/*
	*	³õÊ¼»¯
	*/
void strategy::init(straid_t id, strategy_manager* manager)
{
	_id = id;
	_manager = manager;
	this->on_init();
}

estid_t strategy::buy_for_open(const code_t& code,uint32_t count ,double_t price , order_flag flag )
{

	return place_order(OT_OPEN, DT_LONG, code, count, price, flag);
}

estid_t strategy::sell_for_close(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	return place_order(OT_CLOSE, DT_LONG, code, count, price, flag);
}

estid_t strategy::sell_for_open(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	return place_order(OT_OPEN, DT_SHORT, code, count, price, flag);
}

estid_t strategy::buy_for_close(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	return place_order(OT_CLOSE, DT_SHORT,code, count, price, flag);
}

estid_t strategy::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	if (_manager == nullptr)
	{
		return INVALID_ESTID;
	}
	estid_t estid = lt_place_order(_manager->get_lt(), OT_CLOSE, DT_SHORT, code, count, price, flag);
	if(estid != INVALID_ESTID)
	{
		_manager->regist_estid_strategy(estid,this);
	}
	return estid ;
}


void strategy::cancel_order(estid_t order_id)
{
	LOG_DEBUG("cancel_order : %llu\n", order_id);
	if (_manager == nullptr)
	{
		return;
	}
	lt_cancel_order(_manager->get_lt(), order_id);
}

void strategy::set_trading_optimize(uint32_t max_position, trading_optimal opt , bool flag )
{
	if (_manager == nullptr)
	{
		return;
	}
	lt_set_trading_optimize(_manager->get_lt(), max_position, opt, flag);
}

const position_info& strategy::get_position(const code_t& code) const
{
	if (_manager == nullptr)
	{
		return default_position;
	}
	return lt_get_position(_manager->get_lt(),code);
}

const account_info& strategy::get_account() const
{
	if (_manager == nullptr)
	{
		return default_account;
	}
	return lt_get_account(_manager->get_lt());
}

const order_info& strategy::get_order(estid_t order_id) const
{
	if (_manager == nullptr)
	{
		return default_order;
	}
	return lt_get_order(_manager->get_lt(),order_id);
}

void strategy::subscribe(const code_t& code)
{
	if (_manager == nullptr)
	{
		return;
	}
	_manager->regist_code_strategy(code, this);
	lt_subscribe(_manager->get_lt(),code);
}

void strategy::unsubscribe(const code_t& code)
{
	if (_manager == nullptr)
	{
		return;
	}
	lt_unsubscribe(_manager->get_lt(), code);
	_manager->unregist_code_strategy(code, this);
}
time_t strategy::get_last_time() const
{
	if (_manager == nullptr)
	{
		return -1;
	}
	return lt_get_last_time(_manager->get_lt());
}

void strategy::set_cancel_condition(estid_t order_id,std::function<bool(const tick_info*)> callback)
{
	LOG_DEBUG("set_timeout_cancel : %llu\n", order_id);
	if (_manager == nullptr)
	{
		return;
	}
	strategy_manager::_condition_function[order_id] = callback;
	lt_set_cancel_condition(_manager->get_lt(), order_id, strategy_manager::_condition_callback);
}

void strategy::set_trading_filter(std::function<bool()> callback)
{
	if (_manager == nullptr)
	{
		return;
	}
	strategy_manager::_filter_function = callback;
	lt_set_trading_filter(_manager->get_lt(), strategy_manager::_filter_callback);
}

time_t strategy::last_order_time()
{
	if (_manager == nullptr)
	{
		return -1;
	}
	return lt_last_order_time(_manager->get_lt());
}

const order_statistic& strategy::get_order_statistic()
{
	if (_manager == nullptr)
	{
		return default_statistic;
	}
	return lt_get_order_statistic(_manager->get_lt());
}

void* strategy::get_username(size_t size)
{
	if (_manager == nullptr)
	{
		return nullptr;
	}
	return lt_get_userdata(_manager->get_lt(), _id, size);;
}
