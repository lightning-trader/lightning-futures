#pragma once
#include "strategy.h"
#include <random>


class sig_1_strategy : public strategy
{
	enum {
		CLOSE_LONG_ORDER,
		CLOSE_SHORT_ORDER,
		OPEN_LONG_ORDER,
		OPEN_SHORT_ORDER,
		YESTODAY_CLOSE_LONG,
		YESTODAY_CLOSE_SHORT,
		REGARDLESS_CLOSE_LONG,
		REGARDLESS_CLOSE_SHORT,
		TRANSFER_CLOSE_LONG,
		TRANSFER_CLOSE_SHORT,
		ORDER_ESTID_COUNT
	};
	struct persist_data
	{
		uint32_t trading_day;

		estid_t order_estids[ORDER_ESTID_COUNT];

		persist_data() :
			trading_day(0x0U)
		{
			for(size_t i = 0;i < ORDER_ESTID_COUNT;i++)
			{
				order_estids[i] = INVALID_ESTID;
			}
		}
	};

public:
	
	sig_1_strategy(const param& p):
		strategy(),
		_code(p.get<const char*>("code")),
		_open_once(p.get<uint32_t>("open_once")),
		_order_data(nullptr),
		_delta(p.get<uint32_t>("delta")),
		_alpha(p.get<double_t>("alpha")),
		_beta(p.get<double_t>("beta")),
		_yestoday_ratio(p.get<uint32_t>("yestoday_ratio")),
		_coming_to_close(0),
		_random(0, p.get<uint32_t>("random_offset")),
		_expire(p.get<const char*>("expire"))
		{
		};


	~sig_1_strategy(){};


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
	virtual void on_error(error_type type,estid_t localid, const uint32_t error) override;

	/*
	 *	销毁
	 */
	virtual void on_destory()override;

private:
	
	code_t _code ;

	code_t _expire;

	uint32_t _open_once;

	int32_t _delta;

	double_t _alpha;

	double_t _beta;

	uint32_t _yestoday_ratio;

	persist_data* _order_data ;

	tick_info _last_tick;

	time_t _coming_to_close;

	std::default_random_engine _random_engine;

	std::uniform_int_distribution<int> _random;
};

