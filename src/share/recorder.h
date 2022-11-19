// recorder.h: 目标的头文件。
#pragma once
#include "define.h"
#include "event_center.hpp"
#include "data_types.hpp"

class recorder 
{ 
public:
	//订单表
	virtual void record_order_entrust(time_t time, const order_info& order) = 0;
	virtual void record_order_trade(time_t time, estid_t localid) = 0;
	virtual void record_order_cancel(time_t time, estid_t localid, uint32_t last_volume) = 0;

	//仓位表
	virtual void record_position_flow(time_t time, const position_info& position) = 0;

	//资金表
	virtual void record_account_flow(time_t time, const account_info& account) = 0;

};