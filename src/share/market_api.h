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


};

class actual_market : public market_api, public event_source<market_event_type, 1024>
{

};

class dummy_market : public market_api , public event_source<market_event_type, 4>
{

public:

	virtual void play(uint32_t tradeing_day,std::function<void (const tick_info& info)> publish_callback) = 0;

};