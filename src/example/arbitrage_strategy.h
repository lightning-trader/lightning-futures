#pragma once
#include "strategy.h"
#include "receiver.h"
#include "engine.h"
#include <random>



class arbitrage_strategy : public lt::strategy,public lt::tick_receiver
{
	enum class arbitrage_state
	{
		AS_INVALID,	//没有套利机会
		AS_BUY_INTEREST,//可以做买1卖2
		AS_SELL_INTEREST,//可以做卖1买2
	};

	enum class trade_state
	{
		TS_INVALID,
		TS_BUY_SINGLE_TRADE,	//买的已经单腿成交
		TS_BUY_ALREADY_TRADE,	//买的已经双腿成交
		TS_SELL_SINGLE_TRADE,	//卖的已经单腿成交
		TS_SELL_ALREADY_TRADE,	//卖的已经双腿成交
	};

	enum
	{
		PSRDT_BUY_ORDER_1,
		PSRDT_SELL_ORDER_1,
		PSRDT_BUY_ORDER_2,
		PSRDT_SELL_ORDER_2,
		PSRDT_ORDER_COUNT
	};

	struct persist_data
	{
		uint32_t trading_day;
		estid_t order_estids[PSRDT_ORDER_COUNT];
		arbitrage_state a_state ;
		trade_state t_state ;
	};
public:

	arbitrage_strategy(lt::straid_t id, lt::engine& engine, const code_t& code1, const code_t& code2, double_t offset, uint32_t open_once) :
		lt::strategy(id, engine, true, true),
		_code1(code1),
		_code2(code2),
		_price1(.0),
		_price2(.0),
		_open_once(open_once),
		_order_data(nullptr),
		_offset(offset)

	{

	};

	virtual ~arbitrage_strategy()
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
	virtual void on_error(error_type type, estid_t localid, const error_code error) override;

	/*
	 *	销毁
	 */
	virtual void on_destroy(lt::unsubscriber& unsuber)override;

	/*
	 *	每帧调用
	 */
	virtual void on_update() override;

	estid_t try_buy(const code_t& code);

	estid_t try_sell(const code_t& code);

private:

	code_t _code1;

	code_t _code2;

	uint32_t _open_once;

	persist_data* _order_data;

	double_t _price1 ;

	double_t _price2 ;

	double_t _offset ;

};

