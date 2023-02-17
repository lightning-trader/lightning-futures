#pragma once
#include "strategy.h"
#include <random>

class hft_3_strategy : public strategy
{
public:
	
	hft_3_strategy(const code_t& code, int32_t delta,double_t alpha, double_t beta, int32_t random_offset):
		strategy(),
		_code(code),
		_close_long_order(INVALID_ESTID),
		_close_short_order(INVALID_ESTID),
		_open_long_order(INVALID_ESTID),
		_open_short_order(INVALID_ESTID),
		_delta(delta),
		_alpha(alpha),
		_beta(beta),
		_coming_to_close(0),
		_random(0, random_offset)
		
		{
		};


	~hft_3_strategy(){};


public:


	/*
	 *	初始化事件
	 *	生命周期中只会回调一次
	 */
	virtual void on_init() override ;

	/*
	*	交易日初始化完成
	*/
	virtual void on_ready() override;

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
	virtual void on_error(error_type type,estid_t localid, const uint32_t error) override;


private:
	
	code_t _code ;

	int32_t _delta;

	double_t _alpha;

	double_t _beta;

	estid_t _close_long_order ;

	estid_t _close_short_order;

	estid_t _open_long_order ;

	estid_t _open_short_order;


	tick_info _last_tick;

	time_t _coming_to_close;

	std::default_random_engine _random_engine;

	std::uniform_int_distribution<int> _random;
};

