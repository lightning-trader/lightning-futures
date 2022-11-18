// recorder.cpp: 目标的源文件。
//

#include "csv_recorder.h"

void csv_recorder::init(const boost::property_tree::ptree& config)
{

}

void csv_recorder::record_order_entrust(time_t time, const order_info& order)
{

}

void csv_recorder::record_order_trade(time_t time, estid_t localid)
{

}
void csv_recorder::record_order_cancel(time_t time, estid_t localid, uint32_t cancel_volume)
{

}

void csv_recorder::record_position_flow(time_t time, const code_t& code, const position_info& position)
{

}

void csv_recorder::record_account_flow(time_t time, const account_info& account)
{

}