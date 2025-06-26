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
#include "bar_generator.h"
#include <time_utils.hpp>
#include <basic_utils.hpp>

using namespace lt;
bar_generator::bar_generator(const lt::code_t& code, uint32_t period, trading_context*& ctx, const data_wapper& dw, size_t preload_bars) :_code(code), _period(period), _ctx(ctx), _dw(dw), _last_second_change(0), _detail_density(.0)
{
	std::vector<ltd_bar_info> data;
	ltd_error_code res = _dw.get_history_bar(data, _code.to_string().c_str(), static_cast<ltd_period>(_period), _ctx->get_now_time(), preload_bars);
	if(res != ltd_error_code::EC_NO_ERROR)
	{
		PRINT_ERROR("get history bar error :",res);
	}
	for (auto it = data.begin(); it != data.end(); ++it)
	{
		bar_info bar;
		convert_to_bar(bar, *it);
		if(it < data.end()-1)
		{
			_bar_cache.emplace_front(bar);
			_last_second_change = bar.time + period;
		}
		else
		{
			_current_bar = bar;
			_detail_density = bar.detail_density;
		}
		
	}
}
void bar_generator::insert_tick(const tick_info& tick)
{
	seqtm_t time = lt::make_seqtm(_ctx->get_trading_day(),tick.time);
	if(time<_current_bar.time + _period)
	{
		merge_into_bar(tick);
	}
	else
	{
		_next_tick = tick;
	}
}


void bar_generator::add_receiver(bar_receiver * receiver)
{
	auto it = _bar_callback.find(receiver);
	if (it == _bar_callback.end())
	{
		_bar_callback.insert(receiver);
	}
}

void bar_generator::remove_receiver(bar_receiver* receiver)
{
	auto it = _bar_callback.find(receiver);
	if(it != _bar_callback.end())
	{
		_bar_callback.erase(receiver);
	}
}


const std::vector<bar_info> bar_generator::get_kline(size_t length)
{

	std::vector<bar_info> result;
	size_t need_cache = length - 1; //最后一个为current bar
	if(_bar_cache.size()< need_cache)
	{
		std::vector<ltd_bar_info> data;
		seqtm_t previous_time = _bar_cache.begin()->time;
		_dw.get_history_bar(data, _code.to_string().c_str(),static_cast<ltd_period>(_period), previous_time, need_cache - _bar_cache.size());
		for (auto it = data.rbegin(); it != data.rend(); ++it)
		{
			bar_info bar;
			convert_to_bar(bar,*it);
			_bar_cache.emplace_front(bar);
		}
	}
	for (auto it = _bar_cache.begin(); it != _bar_cache.end(); ++it)
	{
		result.emplace_back(*it);
	}
	result.emplace_back(_current_bar);
	return result;
}

bool bar_generator::invalid()const
{
	return _bar_callback.empty();
}

bool bar_generator::poll()
{
	bool result = false ;
	if(_ctx->is_trading_time())
	{
		seqtm_t now = _ctx->get_now_time();
		for (;  _last_second_change < now;  _last_second_change += _period)
		{
			double_t last_price = _current_bar.close;
			_bar_cache.emplace_back(_current_bar);
			//合成
			for (auto it : _bar_callback)
			{
				it->on_bar(_current_bar);
				result = true;
			}
			//初始化下一个bar
			_current_bar.clear();
			_current_bar.id = _code;
			_current_bar.period = _period;
			_current_bar.open = last_price;
			_current_bar.close = last_price;
			_current_bar.high = last_price;
			_current_bar.low = last_price;
			_current_bar.time = _last_second_change;
			_current_bar.volume = 0U;
			_current_bar.detail_density = _detail_density;
			if(!_next_tick.invalid())
			{
				merge_into_bar(_next_tick);
				_next_tick = default_tick;
			}
		}
	}
	return result;
}


void bar_generator::merge_into_bar(const tick_info& tick)
{
	uint32_t delta_volume = static_cast<uint32_t>(tick.volume - _prev_tick.volume);
	_current_bar.id = tick.id;
	_current_bar.period = _period;
	_current_bar.detail_density = _detail_density;
	_current_bar.high = std::max<double_t>(_current_bar.high, tick.price);
	_current_bar.low = std::min<double_t>(_current_bar.low, tick.price);
	_current_bar.close = tick.price;
	_current_bar.volume += delta_volume;

	auto deal_direction = lt::get_deal_direction(_prev_tick, tick);
	if (deal_direction == deal_direction::DD_DOWN)
	{
		//主动卖出
		_current_bar.sell_details[tick.price] += delta_volume;
	}
	else if (deal_direction == deal_direction::DD_UP)
	{
		//主动买出
		_current_bar.buy_details[tick.price] += delta_volume;
	}
	else
	{
		_current_bar.other_details[tick.price] += delta_volume;
	}
	_prev_tick = tick;
}

void bar_generator::convert_to_bar(bar_info& bar, const ltd_bar_info& info)
{
	bar.id = info.code;
	bar.period = info.period;
	bar.open = info.open;
	bar.close = info.close;
	bar.high = info.high;
	bar.low = info.low;
	bar.time = info.time;
	bar.volume = info.volume;
	bar.detail_density = _detail_density;
	for (size_t i = 0U; i < info.price_sell_size; i++)
	{
		if (info.sell_volume[i].price > .0 && info.sell_volume[i].volume > 0U)
		{
			bar.sell_details[info.sell_volume[i].price] += info.sell_volume[i].volume;
		}
	}
	for (size_t i = 0U; i < info.price_buy_size; i++)
	{
		if (info.buy_volume[i].price > .0 && info.buy_volume[i].volume > 0U)
		{
			bar.buy_details[info.buy_volume[i].price] += info.buy_volume[i].volume;
		}
	}
	for (size_t i = 0U; i < info.price_other_size; i++)
	{
		if (info.other_volume[i].price > .0 && info.other_volume[i].volume > 0U)
		{
			bar.other_details[info.other_volume[i].price] += info.other_volume[i].volume;
		}
	}
}