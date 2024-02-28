/*
Distributed under the MIT License(MIT)

Copyright(c) 2023 Jihua Zou EMail: ghuazo@qq.com QQ:137336521

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in the
Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once
#include <define.h>
#include <data_types.hpp>
#include <params.hpp>

namespace lt
{

	class engine;

	class subscriber;

	class unsubscriber;

	typedef untid_t straid_t;

	class strategy
	{
	public:

	private:

		straid_t _id;

		engine& _engine;

		bool _openable;

		bool _closeable;

		daytm_t _coming_to_close;

	public:

		strategy(straid_t id, engine* engine, bool openable, bool closeable);

		virtual ~strategy();
		
		inline straid_t get_id()const
		{
			return _id;
		}
		/*
		*	初始化
		*/
		void init(subscriber& suber);


		/*
		*	周期回调
		*/
		void update();


		/*
		*	销毁
		*/
		void destroy(unsubscriber& unsuber);

		/*
		*	收到消息
		*/
		virtual void handle_change(const std::vector<std::any>& msg) ;

		//回调函数
	private:

		/*
		*	初始化事件
		*	生命周期中只会回调一次
		*/
		virtual void on_init(subscriber& suber) {};

		/*
		 *	销毁
		 */
		virtual void on_destroy(unsubscriber& unsuber) {};

		/*
		*	更新
		*/
		virtual void on_update() {};

		/*
		*	收到消息
		*/
		virtual void on_change(const params& p) {};


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
		virtual void on_deal(estid_t estid, uint32_t deal_volume) {}

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
		virtual void on_error(error_type type, estid_t estid, const error_code error) {}

	protected:
		//功能函数
		/*
		 *	开多单
		 *	code 期货代码 SHFF.rb2301
		 *  price 如果是0表示市价单，其他表示现价单
		 *  flag 默认为正常单
		 *	@estid	本地订单id
		 */
		estid_t buy_for_open(const code_t& code, uint32_t count, double_t price = 0, order_flag flag = order_flag::OF_NOR);

		/*
		 *	平多单
		 *	code 期货代码 SHFF.rb2301
		 *  price 如果是0表示市价单，其他表示现价单
		 *  flag 默认为正常单
		 *	@estid	本地订单id
		 */
		estid_t sell_for_close(const code_t& code, uint32_t count, double_t price = 0, bool is_close_today = false, order_flag flag = order_flag::OF_NOR);

		/*
		 *	开空单
		 *	code 期货代码 SHFF.rb2301
		 *  price 如果是0表示市价单，其他表示现价单
		 *  flag 默认为正常单
		 *	@estid	本地订单id
		 */
		estid_t sell_for_open(const code_t& code, uint32_t count, double_t price = 0, order_flag flag = order_flag::OF_NOR);

		/*
		 *	平空单
		 *	code 期货代码 SHFF.rb2301
		 *  price 如果是0表示市价单，其他表示现价单
		 *  flag 默认为正常单
		 *	@estid	本地订单id
		 */
		estid_t buy_for_close(const code_t& code, uint32_t count, double_t price = 0, bool is_close_today = false, order_flag flag = order_flag::OF_NOR);


		/*
		 *	撤单
		 *	estid 下单返回的id
		 */
		void cancel_order(estid_t estid);


		/**
		* 获取仓位信息
		*/
		const position_info& get_position(const code_t& code) const;


		/**
		* 获取委托订单
		**/
		const order_info& get_order(estid_t estid) const;

		/**
		* 是否即将收盘
		*/
		bool is_close_coming(daytm_t dtm) const ;

		bool is_close_coming() const ;

		/**
		* 获取时间
		*
		*/
		daytm_t get_last_time() const;

		/**
		* 使用自定义交易通道
		*/
		void use_custom_chain(bool flag);

		/*
		* 设置撤销条件(返回true时候撤销)
		*/
		void set_cancel_condition(estid_t estid, std::function<bool(estid_t)> callback);


		/**
		* 获取最后一次下单时间
		*	跨交易日返回0
		*/
		daytm_t last_order_time();


		/**
		* 获取交易日
		*/
		uint32_t get_trading_day()const;

		
		/*
		* 最新tick
		*/
		const tick_info& get_last_tick(const code_t& code)const;


		/*
		* 获取当前重心价格
		*/
		double_t get_control_price(const code_t& code)const;

		/*
		* 获取下单价格
		*/
		double_t get_proximate_price(const code_t& code,double_t price)const;

		/*
		* 注册订单
		*/
		void regist_order_estid(estid_t estid);

	};
}


extern "C"
{
	EXPORT_FLAG lt::strategy* create_strategy(lt::straid_t id, lt::engine* engine, bool openable, bool closeable, const params& p);
}

