#pragma once
#include <json.hpp>
#include <thread>
#include <event_center.hpp>
#include <trading_context.h>
#include <data_channel.h>
#include <functional>
#include <memory>
#include <crontab_scheduler.hpp>


// Configuration structures for JSON parsing
struct launch_setting
{
    //超时撤单
    uint32_t loop_interval;
    uint32_t daily_open_limit;
    uint32_t daily_order_limit;
    uint32_t order_frequency_limit;
    uint32_t cancel_order_limit;
    
    launch_setting() :loop_interval(0), daily_open_limit(0), daily_order_limit(0), order_frequency_limit(0), cancel_order_limit(0) {}
};

struct ltds_setting
{
    //
    uint32_t kline_cache;
    uint32_t detail_cache;
    std::string channel;//风控服务器
    std::string cache_path;
    ltds_setting() :kline_cache(0), detail_cache(0) {}
};

struct contract_setting
{
	lt::code_t code;
	uint32_t kline_length;
	std::vector<uint32_t> kline_period;
	contract_setting() : kline_length(0) {}
};

namespace lt
{
	class actual_market;
	class actual_trader;
	class message_notify;
}

namespace lt::cta {

	class bridge : trading_context::order_listener,lt::bar_receiver,lt::tick_receiver
	{
		
	private:
		
		lt::trading_context* _ctx ;
		
		lt::data_channel* _dc;

		lt::actual_market* _market;
		
		lt::actual_trader* _trader;

		bool _is_runing;

		uint32_t _trading_day;

		lt::crontab_scheduler _manager;

		std::map<lt::code_t, contract_setting> contract_settings;
		// Python callbacks
		std::function<void(const order_info&)> _on_entrust_callback;
		std::function<void(estid_t, uint32_t)> _on_deal_callback;
		std::function<void(estid_t, const code_t&, offset_type, direction_type, double_t, uint32_t)> _on_trade_callback;
		std::function<void(estid_t, const code_t&, offset_type, direction_type, double_t, uint32_t, uint32_t)> _on_cancel_callback;
		std::function<void(error_type, estid_t, const error_code)> _on_error_callback;
		std::function<void(const lt::tick_info&)> _on_tick_callback;
		std::function<void(const lt::bar_info&)> _on_bar_callback;
	
	private:

	
		virtual void on_tick(const lt::tick_info& tick)  override;
		
		virtual void on_bar(const lt::bar_info& bar) override;
	
	private:

		virtual void on_entrust(const order_info& order) override;

		virtual void on_deal(estid_t estid, uint32_t deal_volume) override;

		virtual void on_trade(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume) override;

		virtual void on_cancel(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume) override;

		virtual void on_error(error_type type, estid_t estid, const error_code error) override;


	public:

		bridge(const std::string& json_config);

		virtual ~bridge();

		void polling();
		// 回调设置
		void set_entrust_callback(std::function<void(const order_info&)> callback) 
		{
			_on_entrust_callback = callback;
		}
		void set_deal_callback(std::function<void(estid_t, uint32_t)> callback) 
		{
			_on_deal_callback = callback;
		}
		void set_trade_callback(std::function<void(estid_t, const code_t&, offset_type, direction_type, double_t, uint32_t)> callback) 
		{
			_on_trade_callback = callback;
		}
		void set_cancel_callback(std::function<void(estid_t, const code_t&, offset_type, direction_type, double_t, uint32_t, uint32_t)> callback) 
		{
			_on_cancel_callback = callback;
		}
		void set_error_callback(std::function<void(error_type, estid_t, const error_code)> callback) 
		{
			_on_error_callback = callback;
		}
		void set_tick_callback(std::function<void(const lt::tick_info&)> callback) 
		{
			_on_tick_callback = callback;
		}
		void set_bar_callback(std::function<void(const lt::bar_info&)> callback) 
		{
			_on_bar_callback = callback;
		}
		
		uint32_t get_trading_day()
		{
			return _ctx->get_trading_day();
		}
		
		bool is_trading(const code_t& code) 
		{
			return _ctx->is_trading(code);
		}
		
		tick_info get_last_tick(const code_t& code)
		{
			return _ctx->get_last_tick(code);
		}
		
		order_statistic get_order_statistic(const code_t& code)
		{
			return _ctx->get_order_statistic(code);
		}

		std::vector<code_t> get_all_contract()const
		{
			std::vector<code_t> result;
			for (const auto& it : contract_settings)
			{
				result.emplace_back(it.first);
			}
			return result;
		}

		const std::vector<bar_info> get_kline(const code_t& code, uint32_t period)
		{
			auto it = contract_settings.find(code);
			if (it != contract_settings.end())
			{
				return _dc->get_kline(code, period, it->second.kline_length);
			}
			return _dc->get_kline(code, period, 100);
		}
		
		const std::vector<tick_info> get_ticks(const code_t& code, uint32_t length)
		{
			return _dc->get_ticks(code, length);
		}
		
		order_info get_order(estid_t estid)
		{
			return _ctx->get_order(estid);
		}
		
		std::vector<order_info> get_all_orders()
		{
			std::vector<order_info> result;
			const auto& orders = _ctx->get_orders();
			for (const auto& it:orders) 
			{
				result.emplace_back(it.second);
			}
			return result;
		}
		
		position_info get_position(const code_t& code)
		{
			return _ctx->get_position(code);
		}
		
		std::vector<position_info> get_all_positions()
		{
			std::vector<position_info> result;
			const auto& positions = _ctx->get_positions();
			for (const auto& it : positions)
			{
				result.emplace_back(it.second);
			}
			return result;
		}
		
		estid_t place_order(const code_t& code, offset_type offset, direction_type direction, uint32_t count,double_t price)
		{
			return _ctx->place_order(this, offset, direction, code, count, price);
		}

		bool cancel_order(estid_t estid) 
		{
			return _ctx->cancel_order(estid);
		}
		
	private:
		
		void check_crossday();

		/*启动*/
		bool start_trading();

		/*停止*/
		bool stop_trading();
	};
}
