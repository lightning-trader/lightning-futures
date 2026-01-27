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
#include <basic_types.hpp>
#include <params.hpp>
#include <chrono>


namespace lt
{
	class market_api;
	class trader_api;

	typedef std::function<bool(const code_t& code, offset_type offset, direction_type direction, uint32_t count, double_t price, order_flag flag)> filter_function;

	class trading_context
	{

	public:


		struct order_listener
		{

			virtual void on_entrust(const order_info& order) = 0;

			virtual void on_deal(estid_t estid, uint32_t deal_volume) = 0;

			virtual void on_trade(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume) = 0;

			virtual void on_cancel(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume) = 0;

			virtual void on_error(error_type type, estid_t estid, const error_code error) = 0;
		};
		
		struct state_listener
		{
			virtual void on_pause(const lt::code_t& product) = 0;
			
			virtual void on_resume(const lt::code_t& product) = 0;
		};

	public:

		trading_context(market_api* market, trader_api* trader, const char* section_file, bool is_simlator=false, state_listener* state_listener=nullptr);

		virtual ~trading_context();

	private:

		trading_context(const trading_context&) = delete;

		trading_context& operator=(const trading_context&) = delete;

		trading_context(trading_context&&) = delete;

		trading_context& operator=(trading_context&&) = delete;

	private:

		//订单事件
		std::map<estid_t, order_listener*> _order_listener;

		state_listener* _state_listener;

		daytm_t _last_order_time;

		std::map<code_t, order_statistic>		_statistic_info;

		std::map<code_t, position_info>			_position_info;

		std::map<estid_t, order_info>			_order_info;

		std::map<code_t, instrument_info>			_instrument_info;

		std::set<estid_t>	_cancel_freeze;

		trader_api* _trader;

		std::map<estid_t,std::pair<std::function<bool(estid_t)>, std::function<void(estid_t)>>> _need_check_condition;

		filter_function _filter_function;

		std::unique_ptr<class time_section> _trading_section ;

		// (实时)
		std::function<void(const tick_info&)> _tick_callback;

		daytm_t _last_tick_time;
		std::chrono::time_point<std::chrono::system_clock> _tick_update_point;
		
		std::map<code_t, tick_info> _previous_tick;

		std::map<code_t, market_info>		_market_info;

		market_api* _market;


	public:

		/*加载数据*/
		bool load_data();
		
		void crossday();

		bool polling();

		estid_t place_order(order_listener* listener, offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag = order_flag::OF_NOR);

		bool cancel_order(estid_t estid);

		const position_info& get_position(const code_t& code)const;

		const order_info& get_order(estid_t estid)const;

		void find_orders(std::vector<order_info>& order_result, std::function<bool(const order_info&)> func) const;

		uint32_t get_total_position() const;

		daytm_t last_order_time()const;

		const order_statistic& get_order_statistic(const code_t& code)const;

		uint32_t get_trading_day()const;

		bool is_trade_time()const;

		bool is_trading(const code_t& code)const;

		uint32_t get_total_pending();

		order_statistic get_all_statistic()const;

		void set_cancel_condition(estid_t estid, std::function<bool(estid_t)> callback, std::function<void(estid_t)> error=nullptr);

		void clear_condition();

		void remove_condition(estid_t estid);

		void set_trading_filter(filter_function callback);

		const instrument_info& get_instrument(const code_t& code)const;

		void regist_order_listener(estid_t estid, order_listener* listener);

		void subscribe(const std::set<code_t>& tick_data, std::function<void(const tick_info&)> tick_callback);

		void unsubscribe(const std::set<code_t>& tick_data);

		daytm_t get_last_time()const;
		
		seqtm_t get_now_time()const;
		//
		const market_info& get_market_info(const code_t& id)const;

		const tick_info& get_last_tick(const code_t& id)const;

		const tick_info& get_previous_tick(const code_t& code)const;

	private:
		
		void update_time(daytm_t time);

		void handle_tick(const std::vector<std::any>& param);

		void check_condition();

		void handle_entrust(const std::vector<std::any>& param);

		void handle_deal(const std::vector<std::any>& param);

		void handle_trade(const std::vector<std::any>& param);

		void handle_cancel(const std::vector<std::any>& param);

		void handle_error(const std::vector<std::any>& param);

		void handle_state(const std::vector<std::any>& param);

		void calculate_position(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume, double_t price);

		void frozen_deduction(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume);

		void unfreeze_deduction(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume);

		void record_pending(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume);

		void recover_pending(const code_t& code, direction_type dir_type, offset_type offset_type, uint32_t volume);

		void print_position(const char* title);
		
	};

}


