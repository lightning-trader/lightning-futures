﻿#pragma once
#include <define.h>


namespace lt
{
	class notify
	{

	public:

		notify() = default;


	public:
		/*
		 *	订单接收回报
		 *  @is_success	是否成功
		 *	@order	本地订单
		 */
		virtual void on_entrust(const order_info& order) {};

		/*
		 *	成交回报
		 *
		 *	@estid	本地订单id
		*/
		virtual void on_deal(estid_t estid, uint32_t deal_volume, uint32_t total_volume) {}

		/*
		 *	成交完成回报
		 *
		 *	@estid	本地订单id
		 */
		virtual void on_trade(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume) {}


		/*
		 *	撤单
		 *	@estid	本地订单id
		 */
		virtual void on_cancel(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume) {}

		/*
		 *	错误
		 *	@estid	本地订单id
		 *	@error 错误代码
		 */
		virtual void on_error(error_type type, estid_t estid, const error_code error) {};
	};
}
