#pragma once
#include "strategy.h"
#include "receiver.h"
#include "engine.h"
#include <random>



class marketing_strategy : public lt::strategy,public lt::tick_receiver
{

	struct persist_data
	{
		uint32_t trading_day;
		estid_t sell_order;
		estid_t buy_order;
	};
public:

	marketing_strategy(lt::straid_t id, lt::engine* engine, const code_t& code, double_t open_detla, uint32_t open_once) :
		lt::strategy(id, engine),
		_code(code),
		_open_once(open_once),
		_open_delta(open_detla),
		_order_data(nullptr),
		_random(0, 1)
	{

	};

	~marketing_strategy()
	{
		_order_data = nullptr;
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
	virtual void on_error(error_type type, estid_t localid, const error_code error) override;

	/*
	 *	销毁
	 */
	virtual void on_destroy(lt::unsubscriber& unsuber)override;

private:

	code_t _code;

	double_t _open_delta;

	uint32_t _open_once;

	persist_data* _order_data;

	std::default_random_engine _random_engine;

	std::uniform_int_distribution<int> _random;
};

