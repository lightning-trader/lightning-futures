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
#include <engine_types.hpp>
#include <event_center.hpp>
#include <receiver.h>
#include <log_wapper.hpp>
#include <context.h>
#include <strategy.h>

namespace lt
{
	enum class deal_direction;

	enum class deal_status;

	struct bar_info;

	struct tape_info;
}

namespace lt::hft
{

	class engine;

	class subscriber
	{

	private:

		engine& _engine;

	public:

		subscriber(engine& engine) :
			_engine(engine)
		{}

		void regist_tick_receiver(const code_t& code, tick_receiver* receiver);
		void regist_tape_receiver(const code_t& code, tape_receiver* receiver);
		void regist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver);

	};

	class unsubscriber
	{

	private:
		engine& _engine;
	public:
		unsubscriber(engine& engine) :
			_engine(engine)
		{}
		void unregist_tick_receiver(const code_t& code, tick_receiver* receiver);
		void unregist_tape_receiver(const code_t& code, tape_receiver* receiver);
		void unregist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver);

	};

	class engine : public context::lifecycle_listener, public queue_event_source<straid_t,1024>
	{
		friend subscriber;
		friend unsubscriber;
		friend strategy;

	private:

		virtual void on_init() override;

		virtual void on_update() override;

		virtual void on_destroy() override;

	protected:

		/***
		* 注册策略
		*/
		void regist_strategy(const std::vector<std::shared_ptr<lt::hft::strategy>>& strategies);

		void clear_strategy();

	public:

		engine();

		~engine();

		/*
		* 设置交易过滤器
		*/
		inline void set_trading_filter(filter_function callback)
		{

			_ctx.set_trading_filter(callback);
		}

		/**
		* 获取当前交易日的订单统计
		*	跨交易日会被清空
		*/
		inline const order_statistic& get_order_statistic(const code_t& code)const
		{
			return _ctx.get_order_statistic(code);
		}

		/**
		* 获取最后一次下单时间
		*	跨交易日返回0
		*/

		inline daytm_t get_last_time()
		{
			return _ctx.get_last_time();
		}

		/**
		* 获取最后一次下单时间
		*	跨交易日返回0
		*/
		inline daytm_t last_order_time()
		{
			return _ctx.last_order_time();
		}

		/**
		* 获取交易日
		*/
		inline uint32_t get_trading_day()const
		{
			return _ctx.get_trading_day();
		}

		/*
		* 获取今日行情数据
		*/
		inline const market_info& get_market_info(const code_t& code)const
		{
			return _ctx.get_market_info(code);
		}

		/*
		* 发送消息
		*/
		inline void change_strategy(straid_t straid,bool openable,bool closeable,const std::string& param)
		{
			this->fire_event(straid, openable, closeable, param);
		}

	private:

		/**
		*	获取策略
		*/
		inline std::shared_ptr<strategy> get_strategy(straid_t id)const
		{
			auto it = _strategy_map.find(id);
			if (it == _strategy_map.end())
			{
				return nullptr;
			}
			return it->second;
		}

		/**
		* 获取交易方向
		*/
		lt::deal_direction get_deal_direction(const lt::tick_info& prev, const lt::tick_info& tick)const;

	protected:

		lt::hft::context _ctx;

		std::map<straid_t, std::shared_ptr<strategy>> _strategy_map;

		std::map<code_t, std::set<tick_receiver*>> _tick_receiver;

		std::map<code_t, std::set<tape_receiver*>> _tape_receiver;

		std::map<code_t, std::map<uint32_t, std::shared_ptr<class bar_generator>>> _bar_generator;

		std::map<code_t,uint32_t> _tick_reference_count ;

		std::map<estid_t, straid_t> _estid_to_strategy;

	};
}
