#include "strategy.h"
#include "lightning.h"
#include "engine.h"

using namespace lt;

strategy::strategy(straid_t id, engine& engine, bool openable, bool closeable):_id(id), _engine(engine), _openable(openable), _closeable(closeable)
{
}
strategy::~strategy()
{
	
}

void strategy::init(subscriber& suber)
{
	this->on_init(suber);
}

void strategy::update()
{
	this->on_update();
}

void strategy::destroy(unsubscriber& unsuber)
{
	this->on_destroy(unsuber);
}

estid_t strategy::buy_for_open(const code_t& code,uint32_t count ,double_t price , order_flag flag )
{
	if(!_openable)
	{
		return INVALID_ESTID ;
	}
	return _engine.place_order(_id, offset_type::OT_OPEN, direction_type::DT_LONG, code, count, price, flag);
}

estid_t strategy::sell_for_close(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	if (!_closeable)
	{
		return INVALID_ESTID;
	}
	return _engine.place_order(_id, offset_type::OT_CLOSE, direction_type::DT_LONG, code, count, price, flag);
}

estid_t strategy::sell_for_open(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	if (!_openable)
	{
		return INVALID_ESTID;
	}
	return _engine.place_order(_id, offset_type::OT_OPEN, direction_type::DT_SHORT, code, count, price, flag);
}

estid_t strategy::buy_for_close(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	if (!_closeable)
	{
		return INVALID_ESTID;
	}
	return _engine.place_order(_id, offset_type::OT_CLOSE, direction_type::DT_SHORT,code, count, price, flag);
}

void strategy::cancel_order(estid_t order_id)
{
	_engine.cancel_order(order_id);
}



const position_info& strategy::get_position(const code_t& code) const
{
	return _engine.get_position(code);
}

const account_info& strategy::get_account() const
{
	return _engine.get_account();
}

const order_info& strategy::get_order(estid_t order_id) const
{
	return _engine.get_order(order_id);
}


time_t strategy::get_last_time() const
{
	return _engine.get_last_time();
}

void strategy::use_custom_chain(bool flag)
{
	return _engine.use_custom_chain( _id, flag);
}

void strategy::set_cancel_condition(estid_t order_id, std::function<bool(const tick_info&)> callback)
{
	return _engine.set_cancel_condition( order_id, callback);
}



time_t strategy::last_order_time()
{
	return _engine.last_order_time();
}


uint32_t strategy::get_trading_day()const
{
	return _engine.get_trading_day();
}

bool strategy::is_trading_ready()const
{
	return _engine.is_trading_ready();
}

const tick_info& strategy::get_last_tick(const code_t& code)const
{
	const auto& market = _engine.get_today_market_info(code);
	if(market.today_tick_info.empty())
	{
		return default_tick_info;
	}
	return *market.today_tick_info.rbegin();
}


uint32_t strategy::get_open_long_pending(const code_t& code)const
{
	return _engine.get_pending_position(code, offset_type::OT_OPEN, direction_type::DT_LONG);
}


uint32_t strategy::get_open_short_pending(const code_t& code)const
{
	return _engine.get_pending_position(code, offset_type::OT_OPEN, direction_type::DT_SHORT);
}


uint32_t strategy::get_close_long_pending(const code_t& code)const
{
	return _engine.get_pending_position(code, offset_type::OT_CLOSE, direction_type::DT_LONG);
}


uint32_t strategy::get_close_short_pending(const code_t& code)const
{
	return _engine.get_pending_position(code, offset_type::OT_CLOSE, direction_type::DT_SHORT);
}