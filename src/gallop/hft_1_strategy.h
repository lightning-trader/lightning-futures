#pragma once
#include "strategy.h"

class hft_1_strategy : public strategy
{
public:
	hft_1_strategy(double open_delta, double close_delta, double lose_delta,int lose_cd_seconds):
		_sell_order(INVALID_ESTID), _buy_order(INVALID_ESTID), _profit_order(INVALID_ESTID), _loss_order(INVALID_ESTID),
		_lose_cd_seconds(lose_cd_seconds), _open_delta(open_delta), _close_delta(close_delta), _lose_delta(lose_delta)
		, _last_lose_time(0)
		{};
	~hft_1_strategy(){};


public:


	/*
	 *	初始化事件
	 *	生命周期中只会回调一次
	 */
	virtual void on_init() override ;

	/*
	 *	tick推送
	 */
	virtual void on_tick(const tick_info* tick)  override;


	/*
	 *	订单接收回报
	 *  @is_success	是否成功
	 *	@localid	本地订单id
	 */
	virtual void on_entrust(estid_t localid) override;

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



private:

	double _open_delta;

	double _close_delta;

	double _lose_delta;

	int _lose_cd_seconds ;

	estid_t _sell_order ;

	estid_t _buy_order ;

	estid_t _profit_order;

	estid_t _loss_order;

	tick_info _last_tick;

	//最后一次止损时间，计算止损cd用
	time_t _last_lose_time;

	time_t _coming_to_close ;
};

