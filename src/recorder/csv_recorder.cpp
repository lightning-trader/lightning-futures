// recorder.cpp: 目标的源文件。
//

#include "csv_recorder.h"
#include <file_wapper.hpp>

csv_recorder::csv_recorder(const char* basic_path) :_is_dirty(false)
{
	if (!file_wapper::exists(basic_path))
	{
		file_wapper::create_directories(basic_path);
	}
	
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