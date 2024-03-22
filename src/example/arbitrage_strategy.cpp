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
#include "arbitrage_strategy.h"
#include "time_utils.hpp"
#include <string_helper.hpp>
#include <mmf_wapper.hpp>

using namespace lt;


void arbitrage_strategy::on_init(subscriber& suber)
{
	suber.regist_tick_receiver(_code1,this);
	suber.regist_tick_receiver(_code2, this);
	use_custom_chain(false);
	uint32_t trading_day = get_trading_day();
	
}

void arbitrage_strategy::on_tick(const tick_info& tick)
{

	if (is_close_coming())
	{
		LOG_DEBUG("time > _coming_to_close", tick.id.get_id(), tick.time);
		return;
	}
	if(tick.id == _code1)
	{
		_price1 = tick.price;
	}
	else if(tick.id == _code2)
	{
		_price2 = tick.price;
	}
	if(_price1 == .0 || _price2 == .0)
	{
		return ;
	}

	if(_price1 - _price2 > _offset)
	{
		//触发了一个市场平衡的信号，
		//买入1号合约 卖出2号合约
		//假设1号合约成交量大于2号合约
		_order_data.a_state = arbitrage_state::AS_BUY_INTEREST;
	}
	else if(_price2 - _price1 > _offset)
	{
		_order_data.a_state = arbitrage_state::AS_SELL_INTEREST;
	}
	else
	{
		_order_data.a_state = arbitrage_state::AS_INVALID;
	}

}



void arbitrage_strategy::on_entrust(const order_info& order)
{
	LOG_INFO("on_entrust :", order.estid, order.code.get_id(), order.direction, order.offset, order.price, order.last_volume, order.total_volume);
	for (size_t i = 0; i < PSRDT_ORDER_COUNT; i++)
	{
		if (_order_data.order_estids[i] == order.estid)
		{
			set_cancel_condition(order.estid, [this](estid_t estid)->bool {

				if (is_close_coming())
				{
					return true;
				}
				return false;
				});
			regist_order_estid(order.estid);
			break;
		}
	}
	
}

void arbitrage_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	LOG_INFO("on_trade :", localid, code.get_id(), direction, offset, price, volume);
	if (_order_data.a_state == arbitrage_state::AS_BUY_INTEREST)
	{
		if(localid == _order_data.order_estids[PSRDT_BUY_ORDER_1])
		{
			if(_order_data.order_estids[PSRDT_SELL_ORDER_2] == INVALID_ESTID)
			{
				//卖2已经成交了
				_order_data.t_state = trade_state::TS_BUY_ALREADY_TRADE;
			}else
			{
				_order_data.t_state = trade_state::TS_BUY_SINGLE_TRADE;
			}
		}
		else if (localid == _order_data.order_estids[PSRDT_SELL_ORDER_2])
		{
			if (_order_data.order_estids[PSRDT_BUY_ORDER_1] == INVALID_ESTID)
			{
				//卖2已经成交了
				_order_data.t_state = trade_state::TS_BUY_ALREADY_TRADE;
			}
			else
			{
				_order_data.t_state = trade_state::TS_BUY_SINGLE_TRADE;
			}
		}
	}
	else if (_order_data.a_state == arbitrage_state::AS_SELL_INTEREST)
	{
		if (localid == _order_data.order_estids[PSRDT_SELL_ORDER_1])
		{
			if (_order_data.order_estids[PSRDT_BUY_ORDER_2] == INVALID_ESTID)
			{
				//卖2已经成交了
				_order_data.t_state = trade_state::TS_SELL_ALREADY_TRADE;
			}
			else
			{
				_order_data.t_state = trade_state::TS_SELL_SINGLE_TRADE;
			}
		}
		else if (localid == _order_data.order_estids[PSRDT_BUY_ORDER_2])
		{
			if (_order_data.order_estids[PSRDT_SELL_ORDER_1] == INVALID_ESTID)
			{
				//卖2已经成交了
				_order_data.t_state = trade_state::TS_SELL_ALREADY_TRADE;
			}
			else
			{
				_order_data.t_state = trade_state::TS_SELL_SINGLE_TRADE;
			}
		}
	}


	for (size_t i = 0; i < PSRDT_ORDER_COUNT; i++)
	{
		if(_order_data.order_estids[i] == localid)
		{
			_order_data.order_estids[i] = INVALID_ESTID;
			break;
		}
	}

}

void arbitrage_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
{
	LOG_INFO("on_cancel :", localid, code.get_id(), direction, offset, price, cancel_volume);

	for (size_t i = 0; i < PSRDT_ORDER_COUNT; i++)
	{
		if (_order_data.order_estids[i] == localid)
		{
			_order_data.order_estids[i] = INVALID_ESTID;
			break;
		}
	}
}

void arbitrage_strategy::on_error(error_type type, estid_t localid, const error_code error)
{
	LOG_ERROR("on_error :", localid, error);
	for (size_t i = 0; i < PSRDT_ORDER_COUNT; i++)
	{
		if (_order_data.order_estids[i] == localid)
		{
			_order_data.order_estids[i] = INVALID_ESTID;
			break;
		}
	}
	
}

void arbitrage_strategy::on_destroy(lt::unsubscriber& unsuber)
{
	unsuber.unregist_tick_receiver(_code1, this);
	unsuber.unregist_tick_receiver(_code2, this);
}

void arbitrage_strategy::on_update()
{
	if (_order_data.a_state == arbitrage_state::AS_BUY_INTEREST)
	{
		if (_order_data.order_estids[PSRDT_BUY_ORDER_1] == INVALID_ESTID)
		{
			_order_data.order_estids[PSRDT_BUY_ORDER_1] = try_buy(_code1);
		}
		if (_order_data.order_estids[PSRDT_SELL_ORDER_2] == INVALID_ESTID)
		{
			_order_data.order_estids[PSRDT_SELL_ORDER_2] = try_sell(_code2);
		}
	}
	else if (_order_data.a_state == arbitrage_state::AS_SELL_INTEREST)
	{
		if (_order_data.order_estids[PSRDT_SELL_ORDER_1] == INVALID_ESTID)
		{
			_order_data.order_estids[PSRDT_SELL_ORDER_1] = try_sell(_code1);
		}
		if (_order_data.order_estids[PSRDT_BUY_ORDER_2] == INVALID_ESTID)
		{
			_order_data.order_estids[PSRDT_BUY_ORDER_2] = try_buy(_code2);
		}
	}
	else if (_order_data.a_state == arbitrage_state::AS_INVALID)
	{
		if (_order_data.t_state == trade_state::TS_BUY_SINGLE_TRADE)
		{
			//套利的止损，平单腿
			if (_order_data.order_estids[PSRDT_BUY_ORDER_1] == INVALID_ESTID)
			{
				//说明买一已经成交了，平调买一单子
				_order_data.order_estids[PSRDT_BUY_ORDER_1] = try_sell(_code1);
			}
			else if (_order_data.order_estids[PSRDT_SELL_ORDER_2] == INVALID_ESTID)
			{
				_order_data.order_estids[PSRDT_SELL_ORDER_2] = try_buy(_code2);
			}
		}
		else if (_order_data.t_state == trade_state::TS_BUY_ALREADY_TRADE)
		{
			//套利盈利，平双腿
			//套利的止损，平单腿
			if (_order_data.order_estids[PSRDT_BUY_ORDER_1] == INVALID_ESTID)
			{
				//说明买一已经成交了，平调买一单子
				_order_data.order_estids[PSRDT_BUY_ORDER_1] = try_sell(_code1);
			}
			if (_order_data.order_estids[PSRDT_SELL_ORDER_2] == INVALID_ESTID)
			{
				_order_data.order_estids[PSRDT_SELL_ORDER_2] = try_buy(_code2);
			}
		}
		else if (_order_data.t_state == trade_state::TS_SELL_SINGLE_TRADE)
		{
			//套利的止损，平单腿
			if (_order_data.order_estids[PSRDT_SELL_ORDER_1] == INVALID_ESTID)
			{
				//说明买一已经成交了，平调买一单子
				_order_data.order_estids[PSRDT_SELL_ORDER_1] = try_buy(_code1);
			}
			else if (_order_data.order_estids[PSRDT_BUY_ORDER_2] == INVALID_ESTID)
			{
				_order_data.order_estids[PSRDT_BUY_ORDER_2] = try_sell(_code2);
			}
		}
		else if (_order_data.t_state == trade_state::TS_SELL_ALREADY_TRADE)
		{
			//套利盈利，平双腿
			//套利的止损，平单腿
			if (_order_data.order_estids[PSRDT_SELL_ORDER_1] == INVALID_ESTID)
			{
				//说明买一已经成交了，平调买一单子
				_order_data.order_estids[PSRDT_SELL_ORDER_1] = try_buy(_code1);
			}
			if (_order_data.order_estids[PSRDT_BUY_ORDER_2] == INVALID_ESTID)
			{
				_order_data.order_estids[PSRDT_BUY_ORDER_2] = try_sell(_code2);
			}
		}
		else
		{
			//双腿未成交，这时候撤单
			for (size_t i = 0; i < PSRDT_ORDER_COUNT; i++)
			{
				if (_order_data.order_estids[i] != INVALID_ESTID)
				{
					cancel_order(_order_data.order_estids[i]);
				}
			}
		}

	}

}

estid_t arbitrage_strategy::try_buy(const code_t& code)
{
	auto& tick = get_last_tick(code);
	auto pos = get_position(code);
	if (pos.history_short.usable() > 0 && _open_once >= pos.history_short.usable())
	{
		return buy_for_close(code, _open_once, tick.sell_price());
	}
	if (pos.today_short.usable() > 0 && _open_once >= pos.today_short.usable())
	{
		return buy_for_close(code, _open_once, tick.sell_price(), true);
	}
	return buy_for_open(code, _open_once, tick.sell_price());
}

estid_t arbitrage_strategy::try_sell(const code_t& code)
{
	auto& tick = get_last_tick(code);
	auto pos = get_position(code);
	if (pos.history_long.usable() > 0 && _open_once >= pos.history_long.usable())
	{
		return sell_for_close(code, _open_once, tick.buy_price());
	}
	if (pos.today_long.usable() > 0 && _open_once >= pos.today_long.usable())
	{
		return sell_for_close(code, _open_once, tick.buy_price(), true);
	}
	return sell_for_open(code, _open_once, tick.buy_price());
}
