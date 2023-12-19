#pragma once
#include "define.h"
#include "event_center.hpp"
#include "data_types.hpp"

enum class market_event_type
{
	MET_Invalid,
	MET_TickReceived,
};
/*
 *	行情解析模块接口
 */
class market_api
{
public:
	
	virtual ~market_api(){}

public:


	/*
	 *	订阅合约列表
	 */
	virtual void subscribe(const std::set<code_t>& codes) = 0;

	/*
	 *	退订合约列表
	 */
	virtual void unsubscribe(const std::set<code_t>& codes) = 0;

	/*
	*	逻辑更新
	*/
	virtual void update() = 0;

	/*
	*	绑定事件
	*/
	virtual void bind_event(market_event_type type,std::function<void(const std::vector<std::any>&)> handle) = 0;

	/*
	*	清理事件
	*/
	virtual void clear_event() = 0;

};

class actual_market : public market_api
{

public:
	
	virtual ~actual_market() {}

	/*
	*	初始化
	*/
	virtual bool login() = 0;

	/*
	*	注销
	*/
	virtual void logout() = 0;


protected:

	std::shared_ptr<std::unordered_map<std::string,std::string>> _id_excg_map ;

	actual_market(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map):_id_excg_map(id_excg_map){}
};

class sync_actual_market : public actual_market,public direct_event_source<market_event_type>
{

protected:
	
	sync_actual_market(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map) :actual_market(id_excg_map){}

	virtual void bind_event(market_event_type type,std::function<void(const std::vector<std::any>&)> handle) override
	{
		this->add_handle(type, handle);
	}

	virtual void clear_event()
	{
		this->clear_handle();
	}
};

class asyn_actual_market : public actual_market,public queue_event_source<market_event_type, 1024>
{

protected:
	
	asyn_actual_market(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map) :actual_market(id_excg_map) {}

	virtual void update()override
	{
		this->process();
	}

	virtual void bind_event(market_event_type type,std::function<void(const std::vector<std::any>&)> handle) override
	{
		this->add_handle(type, handle);
	}

	virtual void clear_event()
	{
		this->clear_handle();
	}
};

class dummy_market : public market_api , public direct_event_source<market_event_type>
{

public:

	virtual ~dummy_market() {}

	/*
	*	绑定事件
	*/
	virtual void bind_event(market_event_type type, std::function<void(const std::vector<std::any>&)> handle) override
	{
		add_handle(type,handle);
	}

	virtual void clear_event()
	{
		this->clear_handle();
	}

public:

	virtual void play(uint32_t trading_day, std::function<void(const tick_info&)> publish_callback) = 0;

	virtual bool is_finished() const = 0;
};