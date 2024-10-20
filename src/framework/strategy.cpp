/*
Distributed under the MIT License(MIT)

Copyright(c) 2023 Jihua Zou EMail: ghuazo@qq.com QQ:137336521

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in the
Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "strategy.h"
#include "time_utils.hpp"
#include "engine.h"

using namespace lt;
using namespace lt::hft;

strategy::strategy(straid_t id, engine* engine, bool openable, bool closeable):_id(id), _engine(*engine),_openable(openable),_closeable(closeable)
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

void strategy::handle_change(const std::vector<std::any>& msg)
{
	if (msg.size() >= 3)
	{
		_openable = std::any_cast<bool>(msg[0]);
		_closeable = std::any_cast<bool>(msg[1]);
		params p(std::any_cast<std::string>(msg[2]));
		this->on_change(p);
		LOG_INFO("strategy change :",get_id(), _openable, _closeable, std::any_cast<std::string>(msg[2]));
	}
}

estid_t strategy::buy_open(const code_t& code,uint32_t count ,double_t price , order_flag flag )
{
	if(!_openable)
	{
		return INVALID_ESTID ;
	}
	return _engine._ctx.place_order(this, offset_type::OT_OPEN, direction_type::DT_LONG, code, count, price, flag);
}

estid_t strategy::sell_close(const code_t& code, uint32_t count, double_t price , bool is_close_today, order_flag flag )
{
	if (!_closeable)
	{
		return INVALID_ESTID;
	}
	if(is_close_today)
	{
		return _engine._ctx.place_order(this, offset_type::OT_CLSTD, direction_type::DT_LONG, code, count, price, flag);
	}
	else
	{
		return _engine._ctx.place_order(this, offset_type::OT_CLOSE, direction_type::DT_LONG, code, count, price, flag);
	}
	
}

estid_t strategy::sell_open(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	if (!_openable)
	{
		return INVALID_ESTID;
	}
	return _engine._ctx.place_order(this, offset_type::OT_OPEN, direction_type::DT_SHORT, code, count, price, flag);
}

estid_t strategy::buy_close(const code_t& code, uint32_t count, double_t price, bool is_close_today, order_flag flag )
{
	if (!_closeable)
	{
		return INVALID_ESTID;
	}
	if(is_close_today)
	{
		return _engine._ctx.place_order(this, offset_type::OT_CLSTD, direction_type::DT_SHORT, code, count, price, flag);
	}
	else
	{
		return _engine._ctx.place_order(this, offset_type::OT_CLOSE, direction_type::DT_SHORT, code, count, price, flag);
	}
	
}

void strategy::cancel_order(estid_t estid)
{

	LOG_DEBUG("cancel_order : ", estid);
	if (_engine._ctx.cancel_order(estid))
	{
		_engine._ctx.remove_condition(estid);
	}
	else
	{
		if (!_engine._ctx.get_order(estid).invalid())
		{
			_engine._ctx.set_cancel_condition(estid, [](estid_t estid)->bool {
				return true;
				});
		}
	}
}

const position_info& strategy::get_position(const code_t& code) const
{
	return _engine._ctx.get_position(code);
}

const order_info& strategy::get_order(estid_t estid) const
{
	return _engine._ctx.get_order(estid);
}


daytm_t strategy::get_last_time() const
{
	return _engine._ctx.get_last_time();
}

void strategy::set_cancel_condition(estid_t estid, std::function<bool(estid_t)> callback)
{
	return _engine._ctx.set_cancel_condition(estid, callback);
}

daytm_t strategy::last_order_time()
{
	return _engine._ctx.last_order_time();
}

uint32_t strategy::get_trading_day()const
{
	return _engine._ctx.get_trading_day();
}

const market_info& strategy::get_market_info(const code_t& code)const
{
	return _engine._ctx.get_market_info(code);
}


double_t strategy::get_proximate_price(const code_t& code, double_t price)const
{
	auto step = _engine._ctx.get_price_step(code);
	return std::round(price / step) * step;
}

double_t strategy::get_price_step(const code_t& code)const
{
	return _engine._ctx.get_price_step(code);
}

void strategy::regist_order_listener(estid_t estid)
{
	_engine._ctx.regist_order_listener(estid,this);
}
