#pragma once
#include <define.h>
#include <data_types.hpp>
#include "event_center.hpp"

//下单接口管理接口
class trader_api
{
public:
	virtual ~trader_api(){}

public:

	/*
	 *	是否可用
	 */
	virtual bool is_usable()const = 0;

	/*
	 *	下单接口
	 *	entrust 下单的具体数据结构
	 */
	virtual estid_t place_order(offset_type offset, direction_type direction , const code_t& code, uint32_t count, double_t price , order_flag flag) = 0;

	/*
	 *	撤单
	 *	action	操作的具体数据结构
	 */
	virtual void cancel_order(estid_t order_id) = 0;


	/*
	 *	提交结算单
	 */
	virtual void submit_settlement() = 0 ;

	/**
	* 获取当前交易日
	*/
	virtual uint32_t get_trading_day()const = 0;

};

class futures_trader : public trader_api, public event_source<32>
{

};
