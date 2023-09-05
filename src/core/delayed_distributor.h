#pragma once
#include "define.h"
#include "log_wapper.hpp"
#include <lightning.h>
#include "trader_api.h"
#include "event_center.hpp"

/***  
* 
* bar 生成器
*/
class delayed_distributor : public event_source<trader_event_type, 1024>
{

private:
	
	//仓位变化（延时）
	position_callback on_position;
	//延时事件可以做一些记录或者通知等IO相关的操作使用
	order_event delayed_event;

private:
	
	void handle_position(const std::vector<std::any>& param);

	void handle_entrust(const std::vector<std::any>& param);

	void handle_deal(const std::vector<std::any>& param);

	void handle_trade(const std::vector<std::any>& param);

	void handle_cancel(const std::vector<std::any>& param);

	void handle_error(const std::vector<std::any>& param);
	
public:

	delayed_distributor(const order_event delayed, position_callback position_cb):
		delayed_event(delayed),
		on_position(position_cb)
	{
		add_handle([this](trader_event_type type, const std::vector<std::any>& param)->void {

			LOG_INFO("event_type", static_cast<uint8_t>(type));
			switch (type)
			{
			
			case trader_event_type::TET_OrderCancel:
				handle_cancel(param);
				break;
			case trader_event_type::TET_OrderPlace:
				handle_entrust(param);
				break;
			case trader_event_type::TET_OrderDeal:
				handle_deal(param);
				break;
			case trader_event_type::TET_OrderTrade:
				handle_trade(param);
				break;
			case trader_event_type::TET_OrderError:
				handle_error(param);
				break;
			}
			});

	}

	void fire(trader_event_type type, const std::vector<std::any>& param)
	{
		fire_event(type, param);
	}
};
