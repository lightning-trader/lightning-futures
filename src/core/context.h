#pragma once
#include <any>
#include <thread>
#include <log_wapper.hpp>
#include <define.h>
#include <lightning.h>
#include "event_center.hpp"
#include <market_api.h>
#include <trader_api.h>
#include <params.hpp>
#include "pod_chain.h"



struct record_data
{
	uint32_t trading_day;
	daytm_t last_order_time;
	order_statistic statistic_info;

	record_data():trading_day(0U),last_order_time(0) {}

	std::ostream& operator>>(std::ostream& os)
	{
		os << trading_day << last_order_time;
		statistic_info>>os;
		return os ;
	}

};

class context
{

public:
	context();
	virtual ~context();

private:
	
	bool _is_runing ;

	// (实时)
	tick_callback _tick_callback;
	ready_callback _ready_callback;
	update_callback _update_callback;
	//实时事件，高频策略使用
	order_event realtime_event;
	
	//实时的线程
	std::thread * _realtime_thread;
	
	daytm_t _last_tick_time;

	uint32_t _max_position;
	
	pod_chain* _default_chain;

	std::unordered_map<untid_t, pod_chain*> _custom_chain;

	filter_callback _trading_filter;

	record_data* _record_data;

	std::atomic<bool> _is_trading_ready ;

	int16_t _bind_cpu_core ;

	uint32_t _loop_interval ;

	std::map<code_t,tick_info> _previous_tick;

	std::map<code_t, today_market_info> _today_market_info;

	position_map			_position_info;

	entrust_map				_order_info;

	std::map<std::string,std::string>	_include_config;
	
public:

	/*加载数据*/
	void load_trader_data();

	/*启动*/
	void start_service() ;
	
	void update();
	/*停止*/
	void stop_service();

	//绑定实时事件
	void bind_realtime_event(const order_event& event_cb, ready_callback ready_cb, update_callback update_cb)
	{
		realtime_event = event_cb;
		this->_ready_callback = ready_cb;
		this->_update_callback = update_cb;
	}

	/*
	* 设置交易过滤器
	*/
	void set_trading_filter(filter_callback callback);

	estid_t place_order(untid_t untid, offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag);
	
	bool cancel_order(estid_t estid);
	
	const position_info& get_position(const code_t& code)const;
	
	const order_info& get_order(estid_t estid)const;

	void find_orders(std::vector<order_info>& order_result, std::function<bool(const order_info&)> func) const;

	uint32_t get_total_position() const;

	void subscribe(const std::set<code_t>& tick_data,tick_callback tick_cb);

	void unsubscribe(const std::set<code_t>& tick_data);
	
	daytm_t get_last_time();

	daytm_t last_order_time();

	const order_statistic& get_order_statistic();

	bool is_trading_ready()
	{
		return _is_trading_ready;
	}

	uint32_t get_trading_day();

	void use_custom_chain(untid_t untid, bool flag);

	inline uint32_t get_max_position()const
	{
		return _max_position;
	}

	inline filter_callback get_trading_filter()const
	{
		return _trading_filter;
	}

	//
	const today_market_info& get_today_market_info(const code_t& id)const;

	uint32_t get_total_pending();

	const tick_info& get_previous_tick(const code_t& code);

	const char* get_include_config(const char* key);

private:

	
	void check_crossday();

	void handle_entrust(const std::vector<std::any>& param);

	void handle_deal(const std::vector<std::any>& param);

	void handle_trade(const std::vector<std::any>& param);

	void handle_cancel(const std::vector<std::any>& param);

	void handle_tick(const std::vector<std::any>& param);

	void handle_error(const std::vector<std::any>& param);

	pod_chain * create_chain(bool flag);

	pod_chain * get_chain(untid_t untid);

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

protected:

	void init(const params& control_config, const params& include_config, bool reset_trading_day=false);

public:

	virtual trader_api& get_trader() = 0;

	virtual market_api& get_market() = 0;

	virtual void on_update() = 0;

	virtual bool is_terminaled() = 0;


};

