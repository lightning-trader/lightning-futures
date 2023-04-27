#include "trading_section.h"
#include <time_utils.hpp>
#include <define.h>

trading_section::trading_section(const std::string &config_path) : _config_csv(config_path, rapidcsv::LabelParams(0, 0))
{

}
trading_section::~trading_section()
{
	
}

void trading_section::init(uint32_t trading_day, time_t last_time)
{
	LOG_INFO("trading_section init %u %s", trading_day,datetime_to_string(last_time).c_str());
	_trading_section.clear();
	time_t trading_day_time = make_date(trading_day);
	time_t last_day_time = get_day_begin(last_time);
	if(trading_day_time != last_day_time)
	{
		// trading_day_time != last_day_time 说明是前天晚上初始化的
		for (size_t i = 0; i < _config_csv.GetRowCount(); i++)
		{
			uint32_t is_day = _config_csv.GetCell<uint32_t>("day_or_night", i);
			const std::string& begin_time_str = _config_csv.GetCell<std::string>("begin", i);
			const std::string& end_time_str = _config_csv.GetCell<std::string>("end", i);
			if (is_day > 0)
			{
				time_t begin_time = make_datetime(trading_day_time, begin_time_str.c_str());
				_trading_section[begin_time] = get_next_time(begin_time, end_time_str.c_str());
				LOG_DEBUG("trading_section : %s %s", datetime_to_string(begin_time).c_str(), datetime_to_string(_trading_section[begin_time]).c_str());
			}
			else
			{
				// 夜盘用当前时间计算
				time_t begin_time = make_datetime(last_day_time, begin_time_str.c_str());
				_trading_section[begin_time] = get_next_time(begin_time, end_time_str.c_str());
				LOG_DEBUG("trading_section : %s %s", datetime_to_string(begin_time).c_str(), datetime_to_string(_trading_section[begin_time]).c_str());
			}
		}
	}
	else
	{
		// 第二天白天初始化的 不初始化夜盘（夜盘已经错过了）
		for (size_t i = 0; i < _config_csv.GetRowCount(); i++)
		{
			uint32_t is_day = _config_csv.GetCell<uint32_t>("day_or_night", i);
			const std::string& begin_time_str = _config_csv.GetCell<std::string>("begin", i);
			const std::string& end_time_str = _config_csv.GetCell<std::string>("end", i);
			if (is_day > 0)
			{
				time_t begin_time = make_datetime(trading_day_time, begin_time_str.c_str());
				_trading_section[begin_time] = get_next_time(begin_time, end_time_str.c_str());
				LOG_DEBUG("trading_section : %s %s", datetime_to_string(begin_time).c_str(), datetime_to_string(_trading_section[begin_time]).c_str());
			}
		}
		
	}
}

bool trading_section::is_in_trading(time_t last_time)
{
	LOG_TRACE("trading_section is_in_trading : %s ", datetime_to_string(last_time).c_str());

	for(auto it: _trading_section)
	{
		if(it.first <= last_time && last_time < it.second)
		{
			return true ;
		}
	}
	return false ;
}

time_t trading_section::get_open_time()
{
	auto frist_one = _trading_section.begin();
	if (frist_one == _trading_section.end())
	{
		return 0;
	}
	return frist_one->second;
}

time_t trading_section::get_clase_time()
{
	auto last_one = _trading_section.rbegin();
	if(last_one == _trading_section.rend())
	{
		return 0;
	}
	return last_one->second;
}

