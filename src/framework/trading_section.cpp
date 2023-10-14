#include "trading_section.h"
#include <define.h>
#include <rapidcsv.h>
#include "log_wapper.hpp"
#include <time_utils.hpp>

trading_section::trading_section(const std::string& config_path)
{
	LOG_INFO("trading_section init ");
	_trading_section.clear();
	rapidcsv::Document config_csv(config_path, rapidcsv::LabelParams(0, 0));
	for (size_t i = 0; i < config_csv.GetRowCount(); i++)
	{
		uint32_t is_day = config_csv.GetCell<uint32_t>("day_or_night", i);
		const std::string& begin_time_str = config_csv.GetCell<std::string>("begin", i);
		const std::string& end_time_str = config_csv.GetCell<std::string>("end", i);
		daytm_t begin_time = make_daytm(begin_time_str.c_str(),0U);
		daytm_t end_time = make_daytm(end_time_str.c_str(),0U);
		_trading_section.emplace_back(std::make_pair(begin_time, end_time));
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
	return frist_one->first;
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

daytm_t trading_section::next_open_time(daytm_t now)
{
	size_t i = 0;
	for(;i<_trading_section.size();i++)
	{
		if(_trading_section[i].first <= now)
		{
			break;
		}
	}
	for (size_t j=i; j<i + _trading_section.size(); j++)
	{
		auto iter = _trading_section[j % _trading_section.size()];
		if(now < iter.first)
		{
			return iter.first;
		}
	}
	return 0U;
}