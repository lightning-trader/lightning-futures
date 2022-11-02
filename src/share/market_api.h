#pragma once
#include "define.h"
#include "event_center.hpp"
#include "data_types.hpp"

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

	/***  
	* 获取时间
	*/
	virtual time_t last_tick_time()const = 0;

	/***  
	* 弹出tick信息
	*/
	virtual void pop_tick_info(std::vector<const tick_info*>& result) = 0;

	/**   
	* 获取当前交易日  
	*/
	virtual uint32_t get_trading_day()const = 0 ;
};

class actual_market_api : public market_api,public event_source
{
public:

	void update()
	{
		handle_event();
	}
};
