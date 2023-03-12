#pragma once
#include <define.h>
#include <any>
#include <recorder.h>
#include <lightning.h>
#include <thread>
#include "event_center.hpp"
#include "market_api.h"
#include "trader_api.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "trading_section.h"
#include "pod_chain.h"

struct record_data
{
	uint32_t trading_day;
	time_t last_order_time;
	order_statistic statistic_info;

	record_data():trading_day(0U),last_order_time(0) {}

};

struct transfer_info
{
	code_t expire_code;
	double_t price_offset ;

	transfer_info():price_offset(.0F)
	{}

	transfer_info(const code_t expire,double_t offset):expire_code(expire), price_offset(offset)
	{}
};

class context
{

public:
	context();
	virtual ~context();

private:
	
	bool _is_runing ;
	
	std::thread * _strategy_thread;

	uint32_t _max_position;
	
	pod_chain* _default_chain;

	std::unordered_map<untid_t, pod_chain*> _custom_chain;
	
	std::map<estid_t, condition_callback> _need_check_condition;

	filter_callback _trading_filter;

	record_data* _record_data;

	std::shared_ptr<boost::interprocess::mapped_region> _record_region;

	size_t _userdata_size ;

	std::vector<std::shared_ptr<boost::interprocess::mapped_region>> _userdata_region ;

	recorder* _recorder ;

	bool _is_trading_ready ;

	std::shared_ptr<trading_section> _section ;

	bool _fast_mode ;

	uint32_t _loop_interval ;

	std::map<code_t, transfer_info> _transfer_map ;

public:

	tick_callback on_tick ;

	entrust_callback on_entrust ;

	deal_callback on_deal ;

	trade_callback on_trade ;

	cancel_callback on_cancel ;

	error_callback on_error;

	ready_callback on_ready;

	/*启动*/
	void start_service() ;

	/*停止*/
	void stop_service();

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
	
	const position_info& get_position(const code_t& code);
	
	const account_info& get_account();
	
	const order_info& get_order(estid_t order_id);

	void subscribe(const std::set<code_t>& codes);

	void unsubscribe(const std::set<code_t>& codes);
	
	time_t get_last_time();

	time_t last_order_time();

	const order_statistic& get_order_statistic();

	void* get_userdata(untid_t index, size_t size);

	bool is_trading_ready()
	{
		return _is_trading_ready;
	}

	uint32_t get_trading_day();

	time_t get_close_time();

	void use_custom_chain(untid_t untid, trading_optimal opt, bool flag);

	inline uint32_t get_max_position()const
	{
		return _max_position;
	}

	inline filter_callback get_trading_filter()const
	{
		return _trading_filter;
	}

	void bind_transfer_info(const code_t& code, const code_t& expire,double_t offset);

	const transfer_info* get_transfer_info(const code_t code)const;

private:

	void load_data(const char* localdb_name);

	void handle_account(const std::vector<std::any>& param);

	void handle_position(const std::vector<std::any>& param);

	void handle_crossday(const std::vector<std::any>& param);

	void handle_settlement(const std::vector<std::any>& param);

	void handle_entrust(const std::vector<std::any>& param);

	void handle_deal(const std::vector<std::any>& param);

	void handle_trade(const std::vector<std::any>& param);

	void handle_cancel(const std::vector<std::any>& param);

	void handle_tick(const std::vector<std::any>& param);

	void handle_error(const std::vector<std::any>& param);

	void check_order_condition(const tick_info& tick);

	void remove_invalid_condition(estid_t order_id);

	pod_chain * create_chain(trading_optimal opt, bool flag);

	pod_chain * get_chain(untid_t untid);

	
protected:

	bool init(boost::property_tree::ptree& localdb, boost::property_tree::ptree& include_config, boost::property_tree::ptree& rcd_config);

	virtual void update() = 0;

	virtual void add_handle(std::function<void(event_type, const std::vector<std::any>&)> handle) = 0;

public:

	virtual trader_api& get_trader() = 0;

	virtual market_api& get_market() = 0;


};

