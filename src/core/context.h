#pragma once
#include <define.h>
#include <any>
#include <recorder.h>
#include <lightning.h>
#include <thread>
#include <functional>
#include "event_center.hpp"
#include "market_api.h"
#include "trader_api.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/interprocess/mapped_region.hpp>

struct operational_data
{
	time_t last_order_time;
	order_statistic statistic_info;

	operational_data():last_order_time(0) {}

	void clear()
	{
		last_order_time = 0;
		statistic_info.place_order_amount = 0;
		statistic_info.entrust_amount = 0;
		statistic_info.trade_amount = 0;
		statistic_info.cancel_amount = 0;
	}

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
	
	class pod_chain* _chain;
	
	std::map<estid_t, condition_callback> _need_check_condition;

	filter_callback _trading_filter;

	operational_data* _operational_data;

	std::shared_ptr<boost::interprocess::mapped_region> _operational_region;

	size_t _userdata_block ;

	size_t _userdata_size ;

	std::vector<std::shared_ptr<boost::interprocess::mapped_region>> _userdata_region ;

	recorder* _recorder ;

	std::atomic<bool> _is_trading ;

public:

	tick_callback on_tick ;

	entrust_callback on_entrust ;

	deal_callback on_deal ;

	trade_callback on_trade ;

	cancel_callback on_cancel ;


	/*启动*/
	void start() ;

	/*停止*/
	void stop();

	/*
	* 设置交易管线 
	*/
	void set_trading_optimize(uint32_t max_position, trading_optimal opt, bool flag);

	/*
	* 设置撤销条件
	*/
	void set_cancel_condition(estid_t order_id, condition_callback callback);

	/*
	* 设置交易过滤器
	*/
	void set_trading_filter(filter_callback callback);

	estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag);
	
	void cancel_order(estid_t order_id);
	
	const position_info& get_position(const code_t& code);
	
	const account_info& get_account();
	
	const order_info& get_order(estid_t order_id);

	void subscribe(const std::set<code_t>& codes);

	void unsubscribe(const std::set<code_t>& codes);
	
	time_t get_last_time();

	time_t last_order_time();

	const order_statistic& get_order_statistic();

	void* get_userdata(uint32_t index, size_t size);

	bool is_in_trading()
	{
		return _is_trading;
	}

private:

	void load_data(const char* localdb_name, size_t oper_size);

	void handle_account(const std::vector<std::any>& param);

	void handle_position(const std::vector<std::any>& param);

	void handle_crossday(const std::vector<std::any>& param);

	void handle_begin(const std::vector<std::any>& param);

	void handle_end(const std::vector<std::any>& param);

	void handle_entrust(const std::vector<std::any>& param);

	void handle_deal(const std::vector<std::any>& param);

	void handle_trade(const std::vector<std::any>& param);

	void handle_cancel(const std::vector<std::any>& param);

	void handle_tick(const std::vector<std::any>& param);

	void check_order_condition(const tick_info& tick);

	void remove_invalid_condition(estid_t order_id);


protected:

	bool init(boost::property_tree::ptree& localdb, boost::property_tree::ptree& rcd_config);

	virtual class trader_api* get_trader() = 0;

	virtual class market_api* get_market() = 0;

	virtual void update() = 0;

	virtual void add_handle(std::function<void(event_type, const std::vector<std::any>&)> handle) = 0;

};

