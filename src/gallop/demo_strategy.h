#pragma once
#include "../core/strategy.h"

class demo_strategy : public strategy
{
public:
	demo_strategy(int lose_offset,int open_delta):_lose_offset(lose_offset), _open_delta(open_delta), _last_order_time(0), _long_lose_price(0), _short_lose_price(0) {};
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
	virtual void on_trade(estid_t localid, code_t code, offset_type offset, direction_type direction, double_t price, uint32_t volume)  override;


	/*
	 *	撤单
	 *	@localid	本地订单id
	 */
	virtual void on_cancel(estid_t localid, code_t code, offset_type offset, direction_type directionv, double_t price, uint32_t cancel_volume, uint32_t total_volume)  override;

private:

	bool check_lose(const tick_info* tick);

private:

	int _lose_offset;

	int _open_delta;

	estid_t _sell_order ;

	estid_t _buy_order ;

	time_t _last_order_time;
	double _long_lose_price; //止损价
	double _short_lose_price; //止损价

	std::list<double_t> _history_list ;

};

