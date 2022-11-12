#pragma once
#include <define.h>
#include <any>
#include <lightning.h>
#include <thread>
#include <functional>

class context : public ltobj
{

public:
	context();
	virtual ~context();

private:
	
	bool _is_runing ;
	
	std::thread * _strategy_thread;

	uint32_t _max_position;
	
	class pod_chain* _chain;
	
	std::map<estid_t, std::function<bool(const tick_info*)>> _need_check_condition;

protected:

	class market_api* _market;

	class trader_api* _trader;

public:

	on_tick_callback on_tick ;

	on_entrust_callback on_entrust ;

	on_deal_callback on_deal ;

	on_trade_callback on_trade ;

	on_cancel_callback on_cancel ;

	/*启动*/
	void start() ;

	/*停止*/
	void stop();

	/*
	下单
	*/
	estid_t place_order(offset_type offset, direction_type direction, code_t code, uint32_t count, double_t price, order_flag flag);

	/*
	* 撤销订单
	*/
	void cancel_order(estid_t order_id);

	/*
	* 设置交易管线 
	*/
	void set_trading_optimize(uint32_t max_position, trading_optimal opt, bool flag);

	/**
	* 获取仓位信息
	*/
	const position_info* get_position(code_t code) const;

	/**
	* 获取账户资金
	*/
	const account_info* get_account() const;

	/**
	* 获取委托订单
	**/
	const order_info* get_order(estid_t order_id) const;


	/**
	* 订阅行情
	**/
	void subscribe(const std::set<code_t>& codes);

	/**
	* 取消订阅行情
	**/
	void unsubscribe(const std::set<code_t>& codes);

	/**
	* 获取时间
	*
	*/
	time_t get_last_time() const;

	/*
	* 设置撤销条件
	*/
	void set_cancel_condition(estid_t order_id, std::function<bool(const tick_info*)> callback);

protected:

	virtual void on_update();
	
public:

	void handle_event(event_type type, const std::vector<std::any>& param);

private:

	void run();

	void handle_begin_trading(const std::vector<std::any>& param);

	void handle_end_trading(const std::vector<std::any>& param);

	void handle_entrust(const std::vector<std::any>& param);

	void handle_deal(const std::vector<std::any>& param);

	void handle_trade(const std::vector<std::any>& param);

	void handle_cancel(const std::vector<std::any>& param);

	void handle_tick(const tick_info* tick);

	void check_order_condition(const tick_info* tick);

	void remove_invalid_condition(estid_t order_id);

};

