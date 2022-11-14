#include "strategy.h"
#include <log_wapper.hpp>
#include "lightning.h"

strategy::strategy():_lt(CT_INVALID)
{
}
strategy::~strategy()
{
	
}
static void tick_p(const tick_info* tick)
{

}
/*
	*	³õÊ¼»¯
	*/
void strategy::init(ltobj lt)
{
	//_lt = lt;
	//tick_callback tick_cb = std::bind(&strategy::on_tick, this, std::placeholders::_1);
	tick_callback tick_cb = (tick_callback)((uint64_t)this + (uint64_t)(&strategy::on_tick));

	lt_bind_callback(_lt
		, std::bind(this+&strategy::on_tick,this, std::placeholders::_1)
		, std::bind(&strategy::on_entrust, this, std::placeholders::_1)
		, std::bind(&strategy::on_deal, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
		, std::bind(&strategy::on_trade, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6)
		, std::bind(&strategy::on_cancel, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7)
	);
	this->on_init();
}

estid_t strategy::buy_for_open(code_t code,uint32_t count ,double_t price , order_flag flag )
{
	return lt_place_order(_lt,OT_OPEN, DT_LONG, code, count, price, flag);
}


estid_t strategy::sell_for_close(code_t code, uint32_t count, double_t price , order_flag flag )
{
	return lt_place_order(_lt, OT_CLOSE, DT_LONG, code, count, price, flag);
}

estid_t strategy::sell_for_open(code_t code, uint32_t count, double_t price , order_flag flag )
{
	return lt_place_order(_lt, OT_OPEN, DT_SHORT, code, count, price, flag);
}

estid_t strategy::buy_for_close(code_t code, uint32_t count, double_t price , order_flag flag )
{
	return lt_place_order(_lt, OT_CLOSE, DT_SHORT,code, count, price, flag);
}

void strategy::cancel_order(estid_t order_id)
{
	LOG_DEBUG("cancel_order : %s\n", order_id.to_str());
	lt_cancel_order(_lt, order_id);
}

void strategy::set_trading_optimize(uint32_t max_position, trading_optimal opt , bool flag )
{
	lt_set_trading_optimize(_lt, max_position, opt, flag);
}

const position_info* strategy::get_position(code_t code) const
{
	return lt_get_position(_lt,code);
}

const account_info* strategy::get_account() const
{
	return lt_get_account(_lt);
}

const order_info* strategy::get_order(estid_t order_id) const
{
	return lt_get_order(_lt,order_id);
}

void strategy::subscribe(code_t code) 
{
	lt_subscribe(_lt,code);
}

void strategy::unsubscribe(code_t code)
{
	lt_unsubscribe(_lt, code);
}
time_t strategy::get_last_time() const
{
	return lt_get_last_time(_lt);
}

void strategy::set_cancel_condition(estid_t order_id,std::function<bool(const tick_info*)> callback)
{
	LOG_DEBUG("set_timeout_cancel : %s\n", order_id.to_str());
	lt_set_cancel_condition(_lt, order_id, callback);
}

