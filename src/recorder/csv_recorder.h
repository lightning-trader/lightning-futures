#pragma once
#include "recorder.h"
#include <boost/property_tree/ptree.hpp>

class csv_recorder : public recorder
{
private:
	
public :
	
	void init(const boost::property_tree::ptree& config);

public:
	//订单表
	virtual void record_order_entrust(time_t time, const order_info& order) override;
	virtual void record_order_trade(time_t time, estid_t localid) override;
	virtual void record_order_cancel(time_t time, estid_t localid, uint32_t cancel_volume) override;

	//仓位表
	virtual void record_position_flow(time_t time, const code_t& code, const position_info& position) override;

	//资金表
	virtual void record_account_flow(time_t time, const account_info& account) override;

};