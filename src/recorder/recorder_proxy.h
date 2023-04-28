#pragma once
#include "recorder.h"
#include <define.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <thread>

class recorder_proxy : public recorder
{
private:

	typedef enum order_action_type
	{
		OAT_ENTRUST,
		OAT_TRADE,
		OAT_CANCEL
	} order_action_type;
private:

	recorder* _recorder ;

	bool _is_reocding ;

	std::thread _record_thread;

	boost::lockfree::spsc_queue<std::pair<order_action_type,std::any>, boost::lockfree::capacity<128>>  _order_lifecycle_queue;
	boost::lockfree::spsc_queue<std::tuple<time_t, position_info>, boost::lockfree::capacity<128>>  _position_flow_queue;
	boost::lockfree::spsc_queue<std::tuple<time_t, account_info>, boost::lockfree::capacity<128>>  _account_flow_queue;
	boost::lockfree::spsc_queue<std::tuple<time_t, uint32_t , order_statistic, account_info>, boost::lockfree::capacity<128>>  _crossday_flow_queue;

public :

	recorder_proxy(recorder* recorder,uint32_t interval);

	void record_update();

	virtual ~recorder_proxy();
	
public:
	//订单表
	virtual void record_order_entrust(time_t time, const order_info& order) override;
	virtual void record_order_trade(time_t time, estid_t localid) override;
	virtual void record_order_cancel(time_t time, estid_t localid, uint32_t last_volume) override;

	//仓位表
	virtual void record_position_flow(time_t time, const position_info& position) override;

	//资金表
	virtual void record_account_flow(time_t time, const account_info& account) override;

	//结算表
	virtual void record_crossday_flow(time_t time, uint32_t trading_day, const order_statistic& statistic, const account_info& account) override;

};