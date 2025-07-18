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

#pragma once
#include <random>
#include "receiver.h"
#include "strategy.hpp"
#include <time_utils.hpp>



class arbitrage_strategy : public lt::hft::strategy,public lt::tick_receiver
{
	enum class arbitrage_state
	{
		AS_INVALID,	//没有套利机会
		AS_BUY_INTEREST,//可以做买1卖2
		AS_SELL_INTEREST,//可以做卖1买2
	};

	enum class trade_state
	{
		TS_INVALID,
		TS_BUY_SINGLE_TRADE,	//买的已经单腿成交
		TS_BUY_ALREADY_TRADE,	//买的已经双腿成交
		TS_SELL_SINGLE_TRADE,	//卖的已经单腿成交
		TS_SELL_ALREADY_TRADE,	//卖的已经双腿成交
	};

	enum
	{
		PSRDT_BUY_ORDER_1,
		PSRDT_SELL_ORDER_1,
		PSRDT_BUY_ORDER_2,
		PSRDT_SELL_ORDER_2,
		PSRDT_ORDER_COUNT
	};

	struct persist_data
	{
		lt::estid_t order_estids[PSRDT_ORDER_COUNT];
		arbitrage_state a_state ;
		trade_state t_state ;

		persist_data() :a_state(arbitrage_state::AS_INVALID), t_state(trade_state::TS_INVALID) {
			for (size_t i = 0; i < PSRDT_ORDER_COUNT; i++)
			{
				order_estids[i] = INVALID_ESTID;
			}
		}
	};
public:

	arbitrage_strategy(lt::hft::straid_t id, lt::hft::syringe* syringe, const lt::code_t& code1, const lt::code_t& code2, double_t offset, uint32_t open_once) :
		lt::hft::strategy(id, syringe),
		_code1(code1),
		_code2(code2),
		_price1(.0),
		_price2(.0),
		_open_once(open_once),
		_offset(offset),
		_order_data()
	{
	};

	virtual ~arbitrage_strategy()
	{
	};


public:


	/*
	 *	初始化事件
	 *	生命周期中只会回调一次
	 */
	virtual void on_init(lt::subscriber& suber) override;

	/*
	 *	tick推送
	 */
	virtual void on_tick(const lt::tick_info& tick)  override;


	/*
	 *	订单接收回报
	 *  @is_success	是否成功
	 *	@order	本地订单
	 */
	virtual void on_entrust(const lt::order_info& order) override;

	/*
	 *	成交回报
	 *
	 *	@localid	本地订单id
	 */
	virtual void on_trade(lt::estid_t localid, const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, double_t price, uint32_t volume)  override;


	/*
	 *	撤单
	 *	@localid	本地订单id
	 */
	virtual void on_cancel(lt::estid_t localid, const lt::code_t& code, lt::offset_type offset, lt::direction_type directionv, double_t price, uint32_t cancel_volume, uint32_t total_volume)  override;

	/*
	 *	错误
	 *	@localid	本地订单id
	 *	@error 错误代码
	 */
	virtual void on_error(lt::error_type type, lt::estid_t localid, const lt::error_code error) override;

	/*
	 *	销毁
	 */
	virtual void on_destroy(lt::unsubscriber& unsuber)override;

	/*
	 *	每帧调用
	 */
	virtual void on_update() override;


private:

	lt::estid_t try_buy(const lt::code_t& code);

	lt::estid_t try_sell(const lt::code_t& code);

	bool is_close_coming() const;
	

private:

	lt::code_t _code1;

	lt::code_t _code2;

	uint32_t _open_once;

	persist_data _order_data;

	double_t _price1 ;

	double_t _price2 ;

	double_t _offset ;

};

