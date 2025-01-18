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
#include <define.h>
#include "ltds_tick_loader.h"
#include <define_types.hpp>
#include <log_define.hpp>

using namespace lt::driver;

ltds_tick_loader::ltds_tick_loader(const std::string& token,const std::string& cache_path,size_t lru_size):_handle(nullptr)
{
	_handle = library_helper::load_library("liblt-data-v3xp");
	ltd_initialize initialize = (ltd_initialize)library_helper::get_symbol(_handle, "ltd_initialize");
	_provider = initialize(cache_path.c_str(), lru_size);
	
}

ltds_tick_loader::~ltds_tick_loader()
{
	ltd_destroy destroy = (ltd_destroy)library_helper::get_symbol(_handle, "ltd_destroy");
	destroy(_provider);
	library_helper::free_library(_handle);
}

void ltds_tick_loader::load_tick(std::vector<tick_detail>& result , const code_t& code, uint32_t trading_day)
{
	std::vector<ltd_tick_info> res;
	res.resize(72000U);
	ltd_get_history_tick get_history_tick = (ltd_get_history_tick)library_helper::get_symbol(_handle, "ltd_get_history_tick");
	size_t real_size = get_history_tick(_provider, res.data(), res.size(), code.to_string().c_str(), trading_day);
	result.resize(real_size);
	for(size_t i=0;i<res.size();i++)
	{
		const ltd_tick_info& it = res[i];	
		tick_detail& tick = result[i];
		tick.id = it.code;
		tick.time = it.time;
		tick.price = it.price;
		tick.volume = it.volume;
		tick.open_interest = it.open_interest;
		tick.trading_day= it.trading_day;
		for(size_t i = 0; i < PRICE_VOLUME_SIZE && i < PRICE_VOLUME_LENGTH;i++)
		{
			tick.bid_order[i] = std::make_pair(it.bid_order[i].price, it.bid_order[i].volume);
		}
		for (size_t i = 0; i < PRICE_VOLUME_SIZE && i < PRICE_VOLUME_LENGTH; i++)
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