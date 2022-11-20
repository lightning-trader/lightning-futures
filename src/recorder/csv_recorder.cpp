﻿// recorder.cpp: 目标的源文件。
//

#include "csv_recorder.h"
#include <file_wapper.hpp>
#include <time_utils.hpp>
#include <boost/lexical_cast.hpp>

csv_recorder::csv_recorder(const char* basic_path) :_is_dirty(false)
{
	if (!file_wapper::exists(basic_path))
	{
		file_wapper::create_directories(basic_path);
	}
	_order_lifecycle_csv.SetColumnName(0,"estid");
	_order_lifecycle_csv.SetColumnName(1, "code");
	_order_lifecycle_csv.SetColumnName(2, "last_volume");
	_order_lifecycle_csv.SetColumnName(3, "total_volume");
	_order_lifecycle_csv.SetColumnName(4, "create_time");
	_order_lifecycle_csv.SetColumnName(5, "finish_time");
	_order_lifecycle_csv.SetColumnName(6, "finish_type");
	_order_lifecycle_csv.SetColumnName(7, "offset");
	_order_lifecycle_csv.SetColumnName(8, "direction");
	_order_lifecycle_csv.SetColumnName(9, "price");
	_order_lifecycle_csv.SetColumnName(10, "flag");

	_position_flow_csv.SetColumnName(0,"time");
	_position_flow_csv.SetColumnName(1, "code");
	_position_flow_csv.SetColumnName(2, "long_postion");
	_position_flow_csv.SetColumnName(3, "short_postion");
	_position_flow_csv.SetColumnName(4, "long_frozen");
	_position_flow_csv.SetColumnName(5, "short_frozen");

	_account_flow_csv.SetColumnName(0, "time");
	_account_flow_csv.SetColumnName(1, "money");
	_account_flow_csv.SetColumnName(2, "frozen");

}

void csv_recorder::record_order_entrust(time_t time, const order_info& order)
{
	size_t count = _order_lifecycle_csv.GetRowCount();
	std::vector<std::string> row_data ;
	row_data.emplace_back(boost::lexical_cast<std::string>(order.est_id));
	row_data.emplace_back(order.code.get_id());
	row_data.emplace_back(boost::lexical_cast<std::string>(order.last_volume));
	row_data.emplace_back(boost::lexical_cast<std::string>(order.total_volume));
	row_data.emplace_back(datetime_to_string(order.create_time));
	row_data.emplace_back(datetime_to_string(0));
	row_data.emplace_back("");
	if(order.offset == offset_type::OT_OPEN)
	{
		row_data.emplace_back("open");
	}
	else
	{
		row_data.emplace_back("close");
	}
	if (order.direction == direction_type::DT_LONG)
	{
		row_data.emplace_back("long");
	}
	else
	{
		row_data.emplace_back("short");
	}
	row_data.emplace_back(boost::lexical_cast<std::string>(order.price));
	if(order.flag == order_flag::OF_FAK)
	{
		row_data.emplace_back("FAK");
	}
	else if(order.flag == order_flag::OF_FOK)
	{
		row_data.emplace_back("FOK");
	}
	else
	{
		row_data.emplace_back("NOR");
	}
	_order_lifecycle_csv.InsertRow<std::string>(count+1, row_data, row_data[0]);
	_order_lifecycle_csv.Save(_basic_path+"/order_lifecycle.csv");
}

void csv_recorder::record_order_trade(time_t time, estid_t localid)
{
	std::string row_name = boost::lexical_cast<std::string>(localid);
	_order_lifecycle_csv.SetCell<uint32_t>("last_volume", row_name,0);
	_order_lifecycle_csv.SetCell<std::string>("finish_time", row_name, datetime_to_string(time));
	_order_lifecycle_csv.SetCell<std::string>("finish_type", row_name, "TRADE");
	_order_lifecycle_csv.Save(_basic_path + "/order_lifecycle.csv");
}
void csv_recorder::record_order_cancel(time_t time, estid_t localid, uint32_t last_volume)
{
	std::string row_name = boost::lexical_cast<std::string>(localid);
	_order_lifecycle_csv.SetCell<uint32_t>("last_volume", row_name, last_volume);
	_order_lifecycle_csv.SetCell<std::string>("finish_time", row_name, datetime_to_string(time));
	_order_lifecycle_csv.SetCell<std::string>("finish_type", row_name, "CANCEL");
	_order_lifecycle_csv.Save(_basic_path + "/order_lifecycle.csv");
}

void csv_recorder::record_position_flow(time_t time, const position_info& position)
{
	size_t count = _position_flow_csv.GetRowCount();
	std::vector<std::string> row_data;
	row_data.emplace_back(datetime_to_string(time));
	row_data.emplace_back(position.id.get_id());
	row_data.emplace_back(boost::lexical_cast<std::string>(position.long_postion));
	row_data.emplace_back(boost::lexical_cast<std::string>(position.short_postion));
	row_data.emplace_back(boost::lexical_cast<std::string>(position.long_frozen));
	row_data.emplace_back(boost::lexical_cast<std::string>(position.short_frozen));
	
	_position_flow_csv.InsertRow<std::string>(count + 1, row_data);
	_position_flow_csv.Save(_basic_path + "/position_flow.csv");
}

void csv_recorder::record_account_flow(time_t time, const account_info& account)
{
	size_t count = _account_flow_csv.GetRowCount();
	std::vector<std::string> row_data;
	row_data.emplace_back(datetime_to_string(time));
	row_data.emplace_back(boost::lexical_cast<std::string>(account.money));
	row_data.emplace_back(boost::lexical_cast<std::string>(account.frozen_monery));
	_account_flow_csv.InsertRow<std::string>(count + 1, row_data);
	_account_flow_csv.Save(_basic_path + "/account_flow.csv");
}