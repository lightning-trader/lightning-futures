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
#include <basic_define.h>
#include "event_center.hpp"
#include <shared_types.h>

namespace lt
{
	enum class trader_event_type : uint8_t
	{
		TET_Invalid,
		TET_OrderCancel,
		TET_OrderPlace,
		TET_OrderDeal,
		TET_OrderTrade,
		TET_OrderError,
		TET_StateChange
	};

	//下单接口管理接口
	class trader_api
	{
	public:

		virtual ~trader_api() {}

	public:

		/*
		 *	是否可用
		 */
		virtual bool is_usable()const = 0;

		/*
		*	逻辑更新
		*/
		virtual bool poll() = 0;

		/*
		 *	下单接口
		 *	entrust 下单的具体数据结构
		 */
		virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) = 0;

		/*
		 *	撤单
		 *	action	操作的具体数据结构
		 */
		virtual bool cancel_order(estid_t estid) = 0;

		/**
		* 获取当前交易日
		*/
		virtual uint32_t get_trading_day()const = 0;

		/**
		* 获取订单数据
		*/
		virtual std::vector<order_info> get_all_orders() = 0;

		/**
		* 获取持仓数据
		*/
		virtual std::vector<position_seed> get_all_positions() = 0;

		/**
		* 获取合约数据
		*/
		virtual std::vector<instrument_info> get_all_instruments() = 0;

		/*
		*	绑定事件
		*/
		virtual void bind_event(trader_event_type type, std::function<void(const std::vector<std::any>&)> handle) = 0;

		/*
		*	清理事件
		*/
		virtual void clear_event() = 0;
	};

	class actual_trader : public trader_api
	{

	public:

		virtual ~actual_trader() {}
		/*
	*	初始化
	*/
		virtual bool login() = 0;

		/*
		*	注销
		*/
		virtual void logout() = 0;


		/*
		*	是否处理完成
		*/
		virtual bool is_idle()const = 0;


		/**
		*	获取交易所时间
		*/
		virtual daytm_t get_exchange_time(const char* exchange) const = 0;

		/**
		*	frist 是否在交易中
		*	second 交易状态开始时间
		*/
		virtual std::pair<bool,daytm_t> get_product_state(const lt::code_t& product_code) const = 0;

		
	protected:

		std::unordered_map<std::string, std::string>& _id_excg_map;

		actual_trader(std::unordered_map<std::string, std::string>& id_excg_map) :_id_excg_map(id_excg_map) {}

		bool is_subscribed(const std::string& code_id)const
		{
			return  _id_excg_map.end() != _id_excg_map.find(code_id);
		}
	};

	class sync_actual_trader : public actual_trader, public direct_event_source<trader_event_type>
	{

	protected:

		sync_actual_trader(std::unordered_map<std::string, std::string>& id_excg_map) :actual_trader(id_excg_map) {}

		virtual bool is_idle()const override
		{
			return true;
		}

		virtual void bind_event(trader_event_type type, std::function<void(const std::vector<std::any>&)> handle) override
		{
			this->add_handle(type, handle);
		}

		virtual void clear_event() override
		{
			this->clear_handle();
		}
	};

	class asyn_actual_trader : public actual_trader, public queue_event_source<trader_event_type, 32768U>
	{

	protected:

		asyn_actual_trader(std::unordered_map<std::string, std::string>& id_excg_map) :actual_trader(id_excg_map) {}

		virtual bool poll()override
		{
			return this->poll();
		}

		virtual bool is_idle()const override
		{
			return this->is_empty();
		}

		virtual void bind_event(trader_event_type type, std::function<void(const std::vector<std::any>&)> handle) override
		{
			this->add_handle(type, handle);
		}

		virtual void clear_event() override
		{
			this->clear_handle();
		}
	};

	class dummy_trader : public trader_api, public direct_event_source<trader_event_type>
	{

	public:

		virtual ~dummy_trader() {}

	public:

		virtual void push_tick(const std::vector<const tick_info*>& current_tick) = 0;

		virtual void crossday(uint32_t trading_day) = 0;

		virtual const account_info& get_account() = 0;

		virtual void bind_event(trader_event_type type, std::function<void(const std::vector<std::any>&)> handle)override
		{
			add_handle(type, handle);
		}

		virtual void clear_event() override
		{
			this->clear_handle();
		}
	};
}

