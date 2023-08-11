#include "trading_section.h"
#include "log_wapper.hpp"
#include <time_utils.hpp>
#include <define.h>

trading_section::trading_section(const std::string& config_path):_config_csv(config_path, rapidcsv::LabelParams(0, 0))
{
	LOG_INFO("trading_section init ");
	_trading_section.clear();
	for (size_t i = 0; i < _config_csv.GetRowCount(); i++)
	{
		uint32_t is_day = _config_csv.GetCell<uint32_t>("day_or_night", i);
		const std::string& begin_time_str = _config_csv.GetCell<std::string>("begin", i);
		const std::string& end_time_str = _config_csv.GetCell<std::string>("end", i);
		daytm_t begin_time = make_daytm(begin_time_str.c_str());
		_trading_section[begin_time] = make_daytm(end_time_str.c_str());
		//LOG_DEBUG("trading_section : ", time_to_string(begin_time), time_to_string(_trading_section[begin_time]));
	}
}
trading_section::~trading_section()
{
	
}


bool trading_section::is_in_trading(daytm_t last_time)
{
	//LOG_TRACE("trading_section is_in_trading : %s ", datetime_to_string(last_time).c_str());

	for(const auto& it: _trading_section)
	{
		if(it.first <= last_time && last_time < it.second)
		{
			return true ;
		}
	}
	return false ;
}


daytm_t trading_section::get_open_time()
{
	auto frist_one = _trading_section.begin();
	if (frist_one == _trading_section.end())
	{
		return 0;
	}
	return frist_one->second;
}

daytm_t trading_section::get_close_time()
{
	auto last_one = _trading_section.rbegin();
	if(last_one == _trading_section.rend())
	{
		return 0;
	}
	return last_one->second;
}

