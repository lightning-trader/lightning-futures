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
#include <log_define.hpp>
#include <define_types.hpp>
#include <params.hpp>

namespace lt{

	class market_api;

	class trader_api;
}

namespace lt::hft
{
	typedef std::function<bool(const code_t& code, offset_type offset, direction_type direction, uint32_t count, double_t price, order_flag flag)> filter_function;

	class context
	{

	public:

		struct lifecycle_listener
		{

			virtual void on_init() = 0;

			virtual void on_update() = 0;

			virtual void on_destroy() = 0;
		};

		struct order_listener
		{

			virtual void on_entrust(const order_info& order) = 0;

			virtual void on_deal(estid_t estid, uint32_t deal_volume) = 0;

			virtual void on_trade(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume) = 0;

			virtual void on_cancel(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume) = 0;

			virtual void on_error(error_type type, estid_t estid, const error_code error) = 0;
		};

	public:

		context(lifecycle_listener* lifecycle);

		virtual ~context();

	private:

		context(const context&) = delete;

		context& operator=(const context&) = delete;

		context(context&&) = delete;

		context& operator=(context&&) = delete;

	private:

		bool _is_runing;
		// (实时)
		std::function<void(const tick_info&)> _tick_callback;

		lifecycle_listener* _lifecycle_listener;

		//订单事件
		std::map<estid_t, order_listener*> _order_listener;

		//实时的线程
		std::thread* _realtime_thread;

		daytm_t _last_tick_time;

		int16_t _bind_cpu_core;

		int16_t _thread_priority;

		uint32_t _loop_interval;

		std::map<code_t, tick_info> _previous_tick;

		daytm_t _last_order_time;

		std::map<code_t, market_info>		_market_info;

		std::map<code_t, order_statistic>		_statistic_info;

		std::map<code_t, position_info>			_position_info;

		std::map<estid_t, order_info>			_order_info;

		std::shared_ptr<class trading_section> _section_config;

		trader_api* _trader;

		market_api* _market;

		std::shared_ptr<class price_step> _ps_config;

		std::map<estid_t, std::function<bool(estid_t)>> _need_check_condition;

		filter_function _filter_function;

	public:

		void init(const params& control_config, const params& include_config, market_api* market, trader_api* trader, bool reset_trading_day = false);

		/*加载数据*/
		bool load_data();

		/*启动*/
		bool start_service();

		void update();
		/*停止*/
		bool stop_service();

		estid_t place_order(order_listener* listener, offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag);

		bool cancel_order(estid_t estid);

		const position_info& get_position(const code_t& code)const;

		const order_info& get_order(estid_t estid)const;

		void find_orders(std::vector<order_info>& order_result, std::function<bool(const order_info&)> func) const;

		uint32_t get_total_position() const;

		void subscribe(const std::set<code_t>& tick_data, std::function<void(const tick_info&)> tick_callback);

		void unsubscribe(const std::set<code_t>& tick_data);

		daytm_t get_last_time();

		daytm_t last_order_time();

		const order_statistic& get_order_statistic(const code_t& code)const;

		uint32_t get_trading_day()const;

		daytm_t get_close_time()const;

		bool is_in_trading()const;
		//
		const market_info& get_market_info(const code_t& id)const;

		uint32_t get_total_pending();

		const tick_info& get_previous_tick(const code_t& code);

		order_statistic get_all_statistic()const;

		void set_cancel_condition(estid_t estid, std::function<bool(estid_t)> callback);

		void clear_condition();

		void remove_condition(estid_t estid);

		void set_trading_filter(filter_function callback);

		double_t get_price_step(const code_t& code)const;

		void regist_order_listener(estid_t estid, order_listener* listener);

	private:

		void check_condition();

		void check_crossday();

		void handle_entrust(const std::vector<std::any>& param);

		void handle_deal(const std::vector<std::any>& param);

		void handle_trade(const std::vector<std::any>& param);

		void handle_cancel(const std::vector<std::any>& param);

		void handle_tick(const std::vector<std::any>& param);

		void handle_error(const std::vector<std::any>& param);

		void calculate_position(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume, double_t price);

		void frozen_deduction(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume);

		void unfreeze_deduction(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume);

		void record_pending(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume);

		void recover_pending(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume);

		inline void print_position(const char* title)
		{
			if (!_position_info.empty())
			{
				LOG_INFO("print_position : ", title);
			}
			for (const auto& it : _position_info)
			{
				const auto& pos = it.second;
				LOG_INFO("position :", pos.id.get_id(), "today_long(", pos.today_long.postion, pos.today_long.frozen, ") today_short(", pos.today_short.postion, pos.today_short.frozen, ") yestoday_long(", pos.history_long.postion, pos.history_long.frozen, ") yestoday_short(", pos.history_short.postion, pos.history_short.frozen, ")");
				LOG_INFO("pending :", pos.id.get_id(), pos.long_pending, pos.short_pending);
			}
		}

	};

}


