#include "strategy.h"
#include <log_wapper.hpp>
#include "lightning.h"

strategy::strategy()
{
	strategy::_self = (this);
}
strategy::~strategy()
{
	
}

/*
	*	³õÊ¼»¯
	*/
void strategy::init(const ltobj& lt)
{
	_lt = lt;

	lt_bind_callback(_lt
		, _tick_callback
		, _entrust_callback
		, _deal_callback
		, _trade_callback
		, _cancel_callback
		);
	this->on_init();
}

estid_t strategy::buy_for_open(const code_t& code,uint32_t count ,double_t price , order_flag flag )
{
	return lt_place_order(_lt,OT_OPEN, DT_LONG, code, count, price, flag);
}

estid_t strategy::sell_for_close(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	return lt_place_order(_lt, OT_CLOSE, DT_LONG, code, count, price, flag);
}

estid_t strategy::sell_for_open(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	return lt_place_order(_lt, OT_OPEN, DT_SHORT, code, count, price, flag);
}

estid_t strategy::buy_for_close(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	return lt_place_order(_lt, OT_CLOSE, DT_SHORT,code, count, price, flag);
}

void strategy::cancel_order(estid_t order_id)
{
	LOG_DEBUG("cancel_order : %llu\n", order_id);
	lt_cancel_order(_lt, order_id);
}

void strategy::set_trading_optimize(uint32_t max_position, trading_optimal opt , bool flag )
{
	lt_set_trading_optimize(_lt, max_position, opt, flag);
}

const position_info& strategy::get_position(const code_t& code) const
{
	return lt_get_position(_lt,code);
}

const account_info& strategy::get_account() const
{
	return lt_get_account(_lt);
}

const order_info& strategy::get_order(estid_t order_id) const
{
	return lt_get_order(_lt,order_id);
}

void strategy::subscribe(const code_t& code)
{
	lt_subscribe(_lt,code);
}

void strategy::unsubscribe(const code_t& code)
{
	lt_unsubscribe(_lt, code);
}
time_t strategy::get_last_time() const
{
	return lt_get_last_time(_lt);
}

void strategy::set_cancel_condition(estid_t order_id,std::function<bool(const tick_info*)> callback)
{
	LOG_DEBUG("set_timeout_cancel : %llu\n", order_id);
	strategy::_condition_function[order_id] = callback;
	lt_set_cancel_condition(_lt, order_id, _condition_callback);
}

