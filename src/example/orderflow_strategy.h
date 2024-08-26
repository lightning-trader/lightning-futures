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
#include "strategy.h"
#include "receiver.h"
#include "engine.h"



class orderflow_strategy : public lt::strategy,public lt::bar_receiver
{

	struct persist_data
	{
		uint32_t trading_day;
		estid_t sell_order;
		estid_t buy_order;
	};
public:

	orderflow_strategy(lt::straid_t id, lt::engine* engine, const code_t& code, uint32_t period, uint32_t open_once, uint32_t multiple, uint32_t threshold, uint32_t position_limit) :
		lt::strategy(id, engine, true, true),
		_code(code),
		_open_once(open_once),
		_period(period),
		_multiple(multiple),
		_threshold(threshold),
		_position_limit(position_limit)
	{
	};

	~orderflow_strategy()
	{
		
	};


public:


	/*
	 *	初始化事件
	 *	生命周期中只会回调一次
	 */
	virtual void on_init(lt::subscriber& suber) override;

	/*
	 *	bar推送
	 */
	virtual void on_bar(const lt::bar_info& bar) override;


	/*
	 *	订单接收回报
	 *  @is_success	是否成功
	 *	@order	本地订单
	 */
	virtual void on_entrust(const order_info& order) override;

	/*
	 *	成交回报
	 *
	 *	@localid	本地订单id
	 */
	virtual void on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)  override;


	/*
	 *	撤单
	 *	@localid	本地订单id
	 */
	virtual void on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type directionv, double_t price, uint32_t cancel_volume, uint32_t total_volume)  override;

	/*
	 *	错误
	 *	@localid	本地订单id
	 *	@error 错误代码
	 */
	virtual void on_error(error_type type, estid_t localid, const error_code error) override;

	/*
	 *	销毁
	 */
	virtual void on_destroy(lt::unsubscriber& unsuber)override;

private:

	bool is_close_coming() const;

private:

	void try_buy();

	void try_sell();

	//合约代码
	code_t _code;

	//一次开仓多少手
	uint32_t _open_once;

	//k线周期（分钟）
	uint32_t _period;

	//失衡delta
	uint32_t _multiple;

	//失衡触发信号的阈值
	uint32_t _threshold;

	//订单限制
	uint32_t _position_limit;

	persist_data _order_data;

};

