#pragma once
#include "recorder.h"
#include <boost/property_tree/ptree.hpp>
#include <rapidcsv.h>

class csv_recorder : public recorder
{
private:
	bool _is_dirty ;
	std::string _basic_path;

	rapidcsv::Document _order_lifecycle_csv ;

	rapidcsv::Document _position_flow_csv;

	rapidcsv::Document _account_flow_csv;

public :

	csv_recorder(const char* basic_path) ;
	
public:
	//订单表
	virtual void record_order_entrust(time_t time, const order_info& order) override;
	virtual void record_order_trade(time_t time, estid_t localid) override;
	virtual void record_order_cancel(time_t time, estid_t localid, uint32_t last_volume) override;

	//仓位表
	virtual void record_position_flow(time_t time, const position_info& position) override;

	//资金表
	virtual void record_account_flow(time_t time, const account_info& account) override;

};