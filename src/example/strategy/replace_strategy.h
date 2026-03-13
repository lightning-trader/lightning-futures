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
#include "strategy.hpp"
#include "receiver.h"



class replace_strategy : public lt::hft::strategy,public lt::tape_receiver
{
	enum {
		LONG_ClOSE_HISTORY,
		LONG_ClOSE_CURRENT,
		LONG_OPEN_CURRENT,
		SHORT_ClOSE_HISTORY,
		SHORT_ClOSE_CURRENT,
		SHORT_OPEN_CURRENT,

		ORDER_ESTID_COUNT
	};

public:

	replace_strategy(lt::hft::straid_t id, lt::hft::syringe* syringe, const lt::code_t& code, uint32_t open_once) :
		lt::hft::strategy(id, syringe),
		_code(code),
		_open_once(open_once), order_estids{INVALID_ESTID}
	{
	};

	~replace_strategy()
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
	virtual void on_tape(const lt::tape_info& tape) override;


	/*
	 *	主线程消息
	 */
	virtual void on_change(const lt::params& p)override;


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


private:

	void try_buy();

	void try_sell();

	void try_close();

	//合约代码
	lt::code_t _code;

	//一次开仓多少手
	uint32_t _open_once;

	lt::estid_t order_estids[ORDER_ESTID_COUNT];

};

