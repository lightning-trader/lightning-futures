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
#include "ltds_tick_loader.h"
#include <fstream>
#include <filesystem>
#include <basic_types.hpp>
#include <string_helper.hpp>
#include <time_utils.hpp>
#include <log_define.hpp>

using namespace lt::driver;

ldts_tick_loader::ldts_tick_loader(const std::string& channel,const std::string& cache_path, size_t detail_cache_size , size_t bar_cache_size )
:_ltd(channel.c_str(), cache_path.c_str(), detail_cache_size, bar_cache_size)
{

}

ldts_tick_loader::~ldts_tick_loader()
{

}

void ldts_tick_loader::load_trading_day(std::vector<uint32_t>& result, uint32_t begin, uint32_t end)
{
	_ltd.get_trading_calendar(result, begin, end);
}


void ldts_tick_loader::load_tick(std::vector<tick_detail>& result , const code_t& code, uint32_t trading_day)
{
	std::vector<ltd_tick_info> res;
	_ltd.get_history_tick(res,code.to_string().c_str(), trading_day);
	for(const auto& it : res)
	{
		tick_detail tick;
		tick.id = it.code;
		tick.time = it.time;
		tick.price = it.price;
		tick.volume = it.volume;
		tick.open_interest = it.open_interest;
		tick.average_price = it.average_price;
		tick.trading_day= it.trading_day;
		for(size_t i=0;i< PRICE_VOLUME_SIZE &&i< WAITING_PRICE_LENGTH;i++)
		{
			tick.bid_order[i] = std::make_pair(it.bid_order[i].price, it.bid_order[i].volume);
		}
		for (size_t i = 0; i < PRICE_VOLUME_SIZE && i < WAITING_PRICE_LENGTH; i++)
		{
			tick.ask_order[i] = std::make_pair(it.ask_order[i].price, it.ask_order[i].volume);
		}
		tick.extend = std::make_tuple(
			it.open,			//open
			it.close,			//close
			it.standard,		//standard
			it.high,		//high
			it.low,			//low
			it.max,			//max
			it.min			//min
		);
	}
}