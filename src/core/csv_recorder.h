#pragma once
#include <define.h>
#include <boost/property_tree/ptree.hpp>
#include <rapidcsv.h>

class csv_recorder 
{
private:

	std::string _basic_path;

	rapidcsv::Document _crossday_flow_csv;

public :

	csv_recorder(const char* basic_path) ;
	
public:

	//结算表
	void record_crossday_flow(time_t time, uint32_t trading_day, const order_statistic& statistic, const account_info& account);

};