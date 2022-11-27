#pragma once
#include "strategy.h"
#include <list>
/*
	策略原理：
	这是一个做市策略，基本原理就是在买一点买入，卖一点价格卖出；
	当价格变动导致自己的价格不是买一价格或者卖一价格时候，执行撤单并且重新下单，保证自己的报单维持在买一和卖一位置
	当买一卖一分别成交以后，执行下一轮操作
	风险分析：
	此策略风险在于价格单边行情，会导致不断亏损，交易所下单和撤单数量限制
	实现思路
	1、由于交易所有撤单数量限制，所以可以通过ltpp的过滤器控制流量
	2、为了使逻辑变得简单，可以通过ltpp开平互转功能，将开仓转成平仓，使我们可以不关注平仓问题，策略中只需要实现开空或者开多
	3、价格变化导致我们的订单不在买一卖一位置时候，通过set_cancel_condition进行撤单，并且撤单后重新下单

*/




class demo_strategy : public strategy
{

public:

	demo_strategy() :
		_offset(1),
		_short_order(INVALID_ESTID),
		_long_order(INVALID_ESTID)
	{};

	demo_strategy(const code_t& code,uint32_t offset):
	_code(code),
	_offset(offset),
	_short_order(INVALID_ESTID), 
	_long_order(INVALID_ESTID)
	{};
	
	~demo_strategy(){};


public:


	/*
	 *	初始化事件
	 *	生命周期中只会回调一次
	 */
	virtual void on_init() override ;

	/*
	 *	tick推送
	 */
	virtual void on_tick(const tick_info& tick)  override;


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
	virtual void on_error(estid_t localid, const uint32_t error)override;


private:

	tick_info _last_tick ;

	uint32_t _offset ;

	code_t _code ;

	// 空仓订单
	estid_t _short_order ;

	// 多仓订单
	estid_t _long_order ;

	
};

