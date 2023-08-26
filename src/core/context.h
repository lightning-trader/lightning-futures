#pragma once
#include <define.h>
#include <any>
#include <recorder.h>
#include <lightning.h>
#include <thread>
#include "event_center.hpp"
#include "market_api.h"
#include <trader_api.h>
#include <params.hpp>
#include "trading_section.h"
#include "pod_chain.h"
#include "bar_generator.h"
#include "delayed_distributor.h"
#include "price_step.h"



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
	bar_callback	_bar_callback;
	ready_callback _ready_callback;
	update_callback _update_callback;
	//实时事件，高频策略使用
	order_event realtime_event;
	
	//实时的线程
	std::thread * _realtime_thread;
	//延时的线程
	std::thread * _delayed_thread;
	std::shared_ptr<delayed_distributor> _distributor;

	daytm_t _last_tick_time;

	uint32_t _max_position;
	
	pod_chain* _default_chain;

	std::unordered_map<untid_t, pod_chain*> _custom_chain;
	
	std::map<estid_t, condition_callback> _need_check_condition;

	filter_callback _trading_filter;

	record_data* _record_data;

	std::atomic<bool> _is_trading_ready ;

	std::shared_ptr<trading_section> _section_config;

	std::shared_ptr<price_step> _price_step_config;

	int16_t _bind_cpu_core ;

	uint32_t _loop_interval ;

	std::map<code_t,tick_info> _previous_tick;
	
	std::map<code_t, std::map<uint32_t, std::shared_ptr<bar_generator>>> _bar_generator;

	std::map<code_t, today_market_info> _today_market_info;

	position_map			_position_info;

	entrust_map				_order_info;

	account_info			_account_info;
	
public:

	/*登录*/
	void login_account();

	/*注销*/
	void logout_account();

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
	//绑定延时事件
	void bind_delayed_event(const order_event& callback,account_callback account_cb,position_callback position_cb)
	{
		_distributor = std::make_shared<delayed_distributor>(callback, account_cb, position_cb);
	}
	/*
	* 设置撤销条件
	*/
	void set_cancel_condition(estid_t order_id, condition_callback callback);

	/*
	* 设置交易过滤器
	*/
	void set_trading_filter(filter_callback callback);

	estid_t place_order(untid_t untid, offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag);
	
	void cancel_order(estid_t order_id);
	
	const position_info& get_position(const code_t& code)const;
	
	const account_info& get_account()const;
	
	const order_info& get_order(estid_t order_id)const;

	void find_orders(std::vector<order_info>& order_result, std::function<bool(const order_info&)> func) const;

	uint32_t get_total_position() const;

	void subscribe(const std::set<code_t>& tick_data,tick_callback tick_cb,const std::map<code_t,std::set<uint32_t>>& bar_data,bar_callback bar_cb);

	void unsubscribe(const std::set<code_t>& tick_data, const std::map<code_t, std::set<uint32_t>>& bar_data);
	
	daytm_t get_last_time();

	daytm_t last_order_time();

	const order_statistic& get_order_statistic();

	bool is_trading_ready()
	{
		return _is_trading_ready;
	}

	uint32_t get_trading_day();

	daytm_t get_close_time()const;

	daytm_t next_open_time(daytm_t time)const;

	bool is_in_trading()const;
	
	bool is_in_trading(daytm_t time)const;
	
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

	uint32_t get_pending_position(const code_t& code, offset_type offset, direction_type direction);

	uint32_t get_open_pending();


private:

	
	void check_crossday();

	void handle_account(const std::vector<std::any>& param);

	void handle_position(const std::vector<std::any>& param);

	void handle_entrust(const std::vector<std::any>& param);

	void handle_deal(const std::vector<std::any>& param);

	void handle_trade(const std::vector<std::any>& param);

	void handle_cancel(const std::vector<std::any>& param);

	void handle_tick(const std::vector<std::any>& param);

	void handle_error(const std::vector<std::any>& param);

	void check_condition();

	void remove_condition(estid_t order_id);

	void clear_condition();

	pod_chain * create_chain(bool flag);

	pod_chain * get_chain(untid_t untid);

	deal_direction get_deal_direction(const tick_info& prev, const tick_info& tick);

protected:

	void init(const params& control_config, const params& include_config, bool reset_trading_day=false);

public:

	virtual trader_api& get_trader() = 0;

	virtual market_api& get_market() = 0;

	virtual void on_update() = 0;

	virtual bool is_terminaled() = 0;

	virtual void add_market_handle(std::function<void(market_event_type, const std::vector<std::any>&)> handle) = 0;

	virtual void add_trader_handle(std::function<void(trader_event_type, const std::vector<std::any>&)> handle) = 0;

};

