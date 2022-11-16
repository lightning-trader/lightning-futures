#pragma once
#include "define.h"
#include "data_types.hpp"


#define LIGHTNING_VERSION 0.0.1


enum context_type
{
	CT_INVALID,
	CT_RUNTIME,
	CT_EVALUATE
};

struct ltobj
{
	void* obj_ptr;

	context_type obj_type;
};

extern "C"
{
	typedef void (PORTER_FLAG * tick_callback)(const tick_info*);

	typedef void (PORTER_FLAG * entrust_callback)(estid_t);

	typedef void (PORTER_FLAG * deal_callback)(estid_t, uint32_t , uint32_t);

	typedef void (PORTER_FLAG * trade_callback)(estid_t, const code_t&, offset_type, direction_type, double_t, uint32_t);

	typedef void (PORTER_FLAG * cancel_callback)(estid_t, const code_t&, offset_type, direction_type, double_t, uint32_t, uint32_t);

	typedef bool (PORTER_FLAG * condition_callback)(estid_t, const tick_info*);

	EXPORT_FLAG ltobj lt_create_context(context_type ctx_type, const char* config_path);
	
	EXPORT_FLAG void lt_destory_context(ltobj& obj);
	
	/*启动*/
	EXPORT_FLAG void lt_start_service(const ltobj& ctx);

	/*停止*/
	EXPORT_FLAG void lt_stop_service(const ltobj& ctx);

	/*
	下单
	*/
	EXPORT_FLAG estid_t lt_place_order(const ltobj& ctx,offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag);

	/*
	* 撤销订单
	*/
	EXPORT_FLAG void lt_cancel_order(const ltobj& ctx,estid_t order_id);

	/*
	* 设置交易管线
	*/
	EXPORT_FLAG void lt_set_trading_optimize(const ltobj& ctx, uint32_t max_position, trading_optimal opt, bool flag);

	/**
	* 获取仓位信息
	*/
	EXPORT_FLAG const position_info& lt_get_position(const ltobj& ctx, const code_t& code) ;

	/**
	* 获取账户资金
	*/
	EXPORT_FLAG const account_info& lt_get_account(const ltobj& ctx) ;

	/**
	* 获取委托订单
	**/
	EXPORT_FLAG const order_info& lt_get_order(const ltobj& ctx, estid_t order_id) ;


	/**
	* 订阅行情
	**/
	EXPORT_FLAG void lt_subscribe(const ltobj& ctx, const code_t& code);

	/**
	* 取消订阅行情
	**/
	EXPORT_FLAG void lt_unsubscribe(const ltobj& ctx, const code_t& code);

	/**
	* 获取时间
	*
	*/
	EXPORT_FLAG time_t lt_get_last_time(const ltobj& ctx);

	/*
	* 设置撤销条件
	*/
	EXPORT_FLAG void lt_set_cancel_condition(const ltobj& ctx, estid_t order_id, condition_callback callback);

	/*
	* 绑定回调 
	*/
	EXPORT_FLAG void lt_bind_callback(const ltobj& ctx, tick_callback tick_cb, entrust_callback entrust_cb, deal_callback deal_cb
	, trade_callback trade_cb, cancel_callback cancel_cb);

	/**
	* 播放历史数据
	*
	*/
	EXPORT_FLAG void lt_playback_history(const ltobj& ctx,uint32_t trading_day);
}
