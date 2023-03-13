#pragma once
#include "strategy.h"

class hcc_strategy : public strategy
{
public:
	hcc_strategy(code_t code,
	float record_ratio,
	int lose_offset,
	int drag_limit,
	int cancel_seconds
	): _code(code), 
	_record_ratio(record_ratio),
	_lose_offset(lose_offset),
	_drag_limit(drag_limit),
	_cancel_seconds(cancel_seconds),
	long_drag(0), 
	short_drag(0),
	current_volume(0),
	_last_order_time(0), 
	_short_lose_price(.0F),
	_long_lose_price(.0F)
	{}
protected:
	/*
	 *	初始化事件
	 *	生命周期中只会回调一次
	 */
	virtual void on_init() override;

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
	virtual void on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)  override;


private:

	void on_price_change(const tick_info& tick, bool is_up);
	
private:

	std::pair<double, int> buy_pending_order;
	std::pair<double, int> sell_pending_order;

	float long_drag;
	float short_drag;
	int current_volume;

	estid_t _order_id ;

	time_t _last_order_time ;

	double _long_lose_price ; //止损价
	double _short_lose_price; //止损价

	code_t _code ;
	float _record_ratio ;
	int _lose_offset ;
	int _drag_limit ;
	int _cancel_seconds;

};

