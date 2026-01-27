/*
Distributed under the MIT License(MIT)

Copyright(c) 2023 Jihua Zou EMail: ghuazo@qq.com QQ:137336521

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in the
Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <basic_define.h>
#include "time_section.h"
#include <rapidcsv.h>
#include "log_define.hpp"
#include <time_utils.hpp>

using namespace lt;

time_section::time_section(const std::string& config_path)
{
	PRINT_INFO("trading_section init ");
	_trading_section.clear();
	rapidcsv::Document config_csv(config_path, rapidcsv::LabelParams(0, 0));
	for (size_t i = 0; i < config_csv.GetRowCount(); i++)
	{
		//uint32_t is_day = config_csv.GetCell<uint32_t>("day_or_night", i);
		const std::string& begin_time_str = config_csv.GetCell<std::string>("begin", i);
		const std::string& end_time_str = config_csv.GetCell<std::string>("end", i);
		daytm_t begin_time = make_daytm(begin_time_str.c_str(),0U);
		daytm_t end_time = make_daytm(end_time_str.c_str(),999U);
		_trading_section.emplace_back(std::make_pair(begin_time, end_time));
		//PRINT_DEBUG("trading_section : ", time_to_string(begin_time), time_to_string(_trading_section[begin_time]));
	}
}
time_section::~time_section()
{
	
}


bool time_section::is_trade_time(daytm_t last_time)const
{
	//PRINT_TRACE("trading_section is_trading : %s ", datetime_to_string(last_time).c_str());

	for(const auto& it: _trading_section)
	{
		if(it.first <= last_time && last_time < it.second)
		{
			return true ;
		}
	}
	return false ;
}


daytm_t time_section::get_open_time()
{
	auto frist_one = _trading_section.begin();
	if (frist_one == _trading_section.end())
	{
		return 0;
	}
	return frist_one->first;
}

daytm_t time_section::get_close_time()
{
	auto last_one = _trading_section.rbegin();
	if(last_one == _trading_section.rend())
	{
		return 0;
	}
	return last_one->second;
}

daytm_t time_section::next_open_time(daytm_t now)
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