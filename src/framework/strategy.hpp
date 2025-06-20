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
#include <params.hpp>
#include <data_channel.h>
#include <log_define.hpp>
#include <basic_types.hpp>

namespace lt::hft
{
	
	typedef uint32_t straid_t;

	struct syringe 
	{
		virtual std::tuple<trading_context*,data_channel*> inject_data() = 0;
	};

	class strategy : trading_context::order_listener
	{
	public:

	private:

		straid_t _id;

		trading_context* _ctx;

		data_channel* _dc;


	public:

		strategy(straid_t id, syringe* syringe) :
			_ctx(nullptr), _dc(nullptr), _id(id)
		{
			if(syringe)
			{
				auto& data = syringe->inject_data();
				_ctx = std::get<0>(data);
				_dc = std::get<1>(data);
			}

		}

		virtual ~strategy(){}
		
		inline straid_t get_id()const
		{
			return _id;
		}

		/*
		*	初始化
		*/
		void init(lt::subscriber& suber)
		{
			this->on_init(suber);
		}

		/*
		*	周期回调
		*/
		void update()
		{
			this->on_update();
		}

		/*
		*	销毁
		*/
		void destroy(lt::unsubscriber& unsuber)
		{
			this->on_destroy(unsuber);
		}

		/*
		*	收到消息
		*/
		virtual void handle_change(const std::vector<std::any>& msg)
		{
			if (msg.size() >= 1)
			{
				params p(std::any_cast<std::string>(msg[0]));
				this->on_change(p);
				LOG_INFO("strategy change :", get_id(), std::any_cast<std::string>(msg[0]));
			}
		}

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
		virtual void on_entrust(const order_info& order) override {};

		/*
		 *	成交回报
		 *
		 *	@estid	本地订单id
		*/
		virtual void on_deal(estid_t estid, uint32_t deal_volume) override {}

		/*
		 *	成交完成回报
		 *
		 *	@estid	本地订单id
		 */
		virtual void on_trade(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume) override {}


		/*
		 *	撤单
		 *	@estid	本地订单id
		 */
		virtual void on_cancel(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume) override {}

		/*
		 *	错误
		 *	@estid	本地订单id
		 *	@error 错误代码
		 */
		virtual void on_error(error_type type, estid_t estid, const error_code error) override {}

	protected:
		//功能函数
		/*
		 *	开多单
		 *	code 期货代码 SHFF.rb2301
		 *  price 如果是0表示市价单，其他表示现价单
		 *  flag 默认为正常单
		 *	@estid	本地订单id
		 */
		estid_t buy_open(const code_t& code, uint32_t count, double_t price = 0, order_flag flag = order_flag::OF_NOR)
		{			
			return _ctx->place_order(this, offset_type::OT_OPEN, direction_type::DT_LONG, code, count, price, flag);
		}

		/*
		 *	平多单
		 *	code 期货代码 SHFF.rb2301
		 *  price 如果是0表示市价单，其他表示现价单
		 *  flag 默认为正常单
		 *	@estid	本地订单id
		 */
		estid_t sell_close(const code_t& code, uint32_t count, double_t price = 0, bool is_close_today = false, order_flag flag = order_flag::OF_NOR)
		{

			if (is_close_today)
			{
				return _ctx->place_order(this, offset_type::OT_CLSTD, direction_type::DT_LONG, code, count, price, flag);
			}
			else
			{
				return _ctx->place_order(this, offset_type::OT_CLOSE, direction_type::DT_LONG, code, count, price, flag);
			}
		}

		/*
		 *	开空单
		 *	code 期货代码 SHFF.rb2301
		 *  price 如果是0表示市价单，其他表示现价单
		 *  flag 默认为正常单
		 *	@estid	本地订单id
		 */
		estid_t sell_open(const code_t& code, uint32_t count, double_t price = 0, order_flag flag = order_flag::OF_NOR)
		{
			return _ctx->place_order(this, offset_type::OT_OPEN, direction_type::DT_SHORT, code, count, price, flag);
		}

		/*
		 *	平空单
		 *	code 期货代码 SHFF.rb2301
		 *  price 如果是0表示市价单，其他表示现价单
		 *  flag 默认为正常单
		 *	@estid	本地订单id
		 */
		estid_t buy_close(const code_t& code, uint32_t count, double_t price = 0, bool is_close_today = false, order_flag flag = order_flag::OF_NOR)
		{
			if (is_close_today)
			{
				return _ctx->place_order(this, offset_type::OT_CLSTD, direction_type::DT_SHORT, code, count, price, flag);
			}
			else
			{
				return _ctx->place_order(this, offset_type::OT_CLOSE, direction_type::DT_SHORT, code, count, price, flag);
			}

		}


		/*
		 *	撤单
		 *	estid 下单返回的id
		 */
		void cancel_order(estid_t estid)
		{
			LOG_DEBUG("cancel_order : ", estid);
			if (_ctx->cancel_order(estid))
			{
				_ctx->remove_condition(estid);
			}
			else
			{
				if (!_ctx->get_order(estid).invalid())
				{
					_ctx->set_cancel_condition(estid, [](estid_t estid)->bool {
						return true;
						});
				}
			}
		}


		/**
		* 获取仓位信息
		*/
		const position_info& get_position(const code_t& code) const
		{
			return _ctx->get_position(code);
		}

		/**
		* 获取委托订单
		**/
		const order_info& get_order(estid_t estid) const
		{
			return _ctx->get_order(estid);
		}

		/**
		* 获取时间
		*
		*/
		daytm_t get_last_time() const
		{
			return _ctx->get_last_time();
		}

		/*
		* 设置撤销条件(返回true时候撤销)
		*/
		void set_cancel_condition(estid_t estid, std::function<bool(estid_t)> callback)
		{
			return _ctx->set_cancel_condition(estid, callback);
		}


		/**
		* 获取交易日
		*/
		uint32_t get_trading_day()const
		{
			return _ctx->get_trading_day();
		}
		
		/*
		* 今日市场信息
		*/
		const market_info& get_market_info(const code_t& code)const
		{
			return _ctx->get_market_info(code);
		}


		/*
		* 获取最后一个tick
		*/
		const tick_info& get_last_tick(const code_t& code)const
		{
			return _ctx->get_last_tick(code);
		}

		/*
		* 合约信息
		*/
		const instrument_info& get_instrument(const code_t& code)const
		{
			return _ctx->get_instrument(code);
		}

		/*
		* 注册订单
		*/
		void regist_order_listener(estid_t estid)
		{
			_ctx->regist_order_listener(estid, this);
		}

	};
}


extern "C"
{
	EXPORT_FLAG lt::hft::strategy* create_strategy(lt::hft::straid_t id, lt::hft::syringe* syringe, const lt::params& p);
}

namespace lt::hft
{
	class strategy_creater
	{

	private:

		typedef strategy* (*create_function)(straid_t, syringe*, const lt::params&);

		dll_handle _strategy_handle;

	public:
		strategy_creater(const std::string& filename)
		{
			_strategy_handle = library_helper::load_library(filename.c_str());
			if (!_strategy_handle)
			{
				LOG_ERROR("load strategy error:", filename);
				throw std::invalid_argument("strategy not find : " + std::string(filename));
			}
		}

		virtual ~strategy_creater()
		{
			if (_strategy_handle)
			{
				library_helper::free_library(_strategy_handle);
				_strategy_handle = nullptr;
			}
		}
		std::shared_ptr<strategy> make_strategy(straid_t id, syringe* syringe, const std::string& param)
		{
			LOG_INFO("make_strategy : ", id, param);
			lt::params p(param);
			create_function creator = (create_function)library_helper::get_symbol(_strategy_handle, "create_strategy");
			if(creator == nullptr)
			{
				LOG_ERROR("cant find create_strategy function :", id);
				throw std::invalid_argument("cant find create_strategy function : " + id);
			}
			auto strategy = creator(id, syringe, p);
			return std::shared_ptr<lt::hft::strategy>(strategy);
		}

	};
}

