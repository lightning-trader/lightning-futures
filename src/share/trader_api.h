#pragma once
#include <define.h>
#include <data_types.hpp>
#include "event_center.hpp"
#include <shared_types.h>


enum class trader_event_type : uint8_t
{
	TET_Invalid,
	TET_OrderCancel,
	TET_OrderPlace,
	TET_OrderDeal,
	TET_OrderTrade,
	TET_OrderError
};

//下单接口管理接口
class trader_api
{
public:
	
	virtual ~trader_api(){}

public:

	/*
	 *	是否可用
	 */
	virtual bool is_usable()const = 0;

	/*
	*	逻辑更新
	*/
	virtual void update() = 0;

	/*
	 *	下单接口
	 *	entrust 下单的具体数据结构
	 */
	virtual estid_t place_order(offset_type offset, direction_type direction , const code_t& code, uint32_t count, double_t price , order_flag flag) = 0;

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
	* 获取交易数据
	*/
	virtual std::shared_ptr<trader_data> get_trader_data() = 0;

	/*
	*	绑定事件
	*/
	virtual void bind_event(trader_event_type type,std::function<void(const std::vector<std::any>&)> handle) = 0;

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


protected:

	std::shared_ptr<std::unordered_map<std::string, std::string>> _id_excg_map;

	actual_trader(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map) :_id_excg_map(id_excg_map) {}

	bool is_subscribed(const std::string& code_id)const 
	{
		return  _id_excg_map&&_id_excg_map->end()!=_id_excg_map->find(code_id);
	}
};

class sync_actual_trader : public actual_trader,public direct_event_source<trader_event_type>
{

protected:

	sync_actual_trader(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map) :actual_trader(id_excg_map) {}

	virtual bool is_idle()const override
	{
		return true;
	}

	virtual void bind_event(trader_event_type type,std::function<void(const std::vector<std::any>&)> handle) override
	{
		this->add_handle(type, handle);
	}

	virtual void clear_event() override
	{
		this->clear_handle();
	}
};

class asyn_actual_trader : public actual_trader,public queue_event_source<trader_event_type, 128>
{

protected:

	asyn_actual_trader(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map) :actual_trader(id_excg_map) {}

	virtual void update()override
	{
		this->process();
	}

	virtual bool is_idle()const override
	{
		return this->is_empty();
	}

	virtual void bind_event(trader_event_type type,std::function<void(const std::vector<std::any>&)> handle) override
	{
		this->add_handle(type, handle);
	}

	virtual void clear_event() override
	{
		this->clear_handle();
	}
};

class dummy_trader : public trader_api , public direct_event_source<trader_event_type>
{

public:

	virtual ~dummy_trader() {}

public:
	
	virtual void push_tick(const tick_info& tick) = 0;

	virtual void crossday(uint32_t trading_day) = 0;

	virtual const account_info& get_account() = 0;

	virtual void bind_event(trader_event_type type,std::function<void(const std::vector<std::any>&)> handle)override
	{
		add_handle(type, handle);
	}

	virtual void clear_event() override
	{
		this->clear_handle();
	}
};