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

};

class futures_market : public market_api, public event_source<64>
{

};
