#pragma once
#include "strategy.h"

class hft_1_strategy : public strategy
{
public:
	
	hft_1_strategy(const code_t& code,double open_delta, double close_delta, double lose_delta,int lose_cd_seconds):
		strategy(),
		_code(code),
		_sell_order(INVALID_ESTID),
		_buy_order(INVALID_ESTID),
		_profit_order(INVALID_ESTID), 
		_loss_order(INVALID_ESTID),
		_lose_cd_seconds(lose_cd_seconds), 
		_open_delta(open_delta),
		_close_delta(close_delta),
		_lose_delta(lose_delta)
		, _last_lose_time(0),
		_coming_to_close(0),
		_history_count(1200),
		_history_ma(0)
		{};


	~hft_1_strategy(){};


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
	virtual void on_tick(const tick_info& tick, const deal_info& deal)  override;


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
	virtual void on_error(error_type type, estid_t localid, const uint32_t error) override;

private:

	void add_to_history(double_t price);

private:
	
	code_t _code ;

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

	size_t _history_count ;

	double_t _history_ma ;

	std::list<double_t> _history_price ;
};

