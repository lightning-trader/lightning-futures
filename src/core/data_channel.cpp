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
#include "data_channel.h"
#include <time_utils.hpp>
#include "bar_generator.h"
#include <log_define.hpp>
#include <basic_utils.hpp>

using namespace lt;

void subscriber::regist_tick_receiver(const code_t& code, tick_receiver* receiver)
{
	auto it = _data_channel._tick_receiver.find(code);
	if (it == _data_channel._tick_receiver.end())
	{
		_data_channel._tick_receiver[code].insert(receiver);
	}
	else
	{
		it->second.insert(receiver);
	}
	_data_channel._tick_reference_count[code]++;
}

void unsubscriber::unregist_tick_receiver(const code_t& code, tick_receiver* receiver)
{
	auto it = _data_channel._tick_receiver.find(code);
	if (it == _data_channel._tick_receiver.end())
	{
		return;
	}
	auto s_it = it->second.find(receiver);
	if (s_it != it->second.end())
	{
		it->second.erase(s_it);
	}
	if (it->second.empty())
	{
		_data_channel._tick_receiver.erase(it);
	}
	auto d_it = _data_channel._tick_reference_count.find(code);
	if (d_it != _data_channel._tick_reference_count.end())
	{
		if (d_it->second > 0)
		{
			d_it->second--;
		}
	}
}

void subscriber::regist_tape_receiver(const code_t& code, tape_receiver* receiver)
{
	auto it = _data_channel._tape_receiver.find(code);
	if (it == _data_channel._tape_receiver.end())
	{
		_data_channel._tape_receiver[code].insert(receiver);
	}
	else
	{
		it->second.insert(receiver);
	}
	_data_channel._tick_reference_count[code]++;
}

void unsubscriber::unregist_tape_receiver(const code_t& code, tape_receiver* receiver)
{
	auto it = _data_channel._tape_receiver.find(code);
	if (it == _data_channel._tape_receiver.end())
	{
		return;
	}
	auto s_it = it->second.find(receiver);
	if (s_it != it->second.end())
	{
		it->second.erase(s_it);
	}
	if (it->second.empty())
	{
		_data_channel._tape_receiver.erase(it);
	}
	auto d_it = _data_channel._tick_reference_count.find(code);
	if (d_it != _data_channel._tick_reference_count.end())
	{
		if (d_it->second > 1)
		{
			d_it->second--;
		}
	}
}

void subscriber::regist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver,size_t preload_bars)
{
	auto generator_iter = _data_channel._bar_generator[code].find(period);
	if (generator_iter == _data_channel._bar_generator[code].end())
	{
		_data_channel._bar_generator[code][period] = std::make_shared<bar_generator>(code,period, _data_channel._ctx,_data_channel._dw, preload_bars);
	}
	_data_channel._bar_generator[code][period]->add_receiver(receiver);
	_data_channel._tick_reference_count[code]++;
}

void subscriber::subscribe()
{
	_data_channel.subscribe();
}

void unsubscriber::unregist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver)
{
	auto it = _data_channel._bar_generator.find(code);
	if (it == _data_channel._bar_generator.end())
	{
		return;
	}
	auto s_it = it->second.find(period);
	if (s_it == it->second.end())
	{
		return;
	}
	s_it->second->remove_receiver(receiver);
	if (s_it->second->invalid())
	{
		it->second.erase(s_it);
		if (it->second.empty())
		{
			_data_channel._bar_generator.erase(it);
		}
	}

	auto d_it = _data_channel._tick_reference_count.find(code);
	if (d_it != _data_channel._tick_reference_count.end())
	{
		if (d_it->second > 1)
		{
			d_it->second--;
		}
	}
}

void unsubscriber::unsubscribe()
{
	_data_channel.unsubscribe();
}

void data_channel::subscribe()
{
	std::set<code_t> tick_subscrib;
	for (auto it = _tick_reference_count.begin(); it != _tick_reference_count.end();)
	{
		if (it->second == 0)
		{
			it = _tick_reference_count.erase(it);
		}
		else
		{
			tick_subscrib.insert(it->first);
			it++;
		}
	}
	_ctx->subscribe(tick_subscrib, [this](const tick_info& tick)->void {
		auto tk_it = _tick_receiver.find(tick.id);
		if (tk_it != _tick_receiver.end())
		{
			for (auto tkrc : tk_it->second)
			{
				if (tkrc)
				{
					PROFILE_DEBUG(tick.id.get_symbol());
					tkrc->on_tick(tick);
					PROFILE_DEBUG(tick.id.get_symbol());
				}
			}
		}

		auto tp_it = _tape_receiver.find(tick.id);
		if (tp_it != _tape_receiver.end())
		{
			for (auto tprc : tp_it->second)
			{
				if (tprc)
				{
					tape_info deal_info(tick.id, tick.time, tick.price);
					const auto& prev_tick = _ctx->get_previous_tick(tick.id);
					deal_info.volume_delta = static_cast<uint32_t>(tick.volume - prev_tick.volume);
					deal_info.interest_delta = tick.open_interest - prev_tick.open_interest;
					deal_info.direction = get_deal_direction(prev_tick, tick);
					tprc->on_tape(deal_info);
				}
			}
		}

		auto br_it = _bar_generator.find(tick.id);
		if (br_it != _bar_generator.end())
		{
			for (auto bg_it : br_it->second)
			{
				bg_it.second->insert_tick(tick);
			}
		}
	});
}

void data_channel::unsubscribe()
{
	std::set<code_t> tick_unsubscrib;
	for (auto it = _tick_reference_count.begin(); it != _tick_reference_count.end();)
	{
		if (it->second == 0)
		{
			tick_unsubscrib.insert(it->first);
			it = _tick_reference_count.erase(it);
		}
		else
		{
			it++;
		}
	}
	_ctx->unsubscribe(tick_unsubscrib);
}

void data_channel::update()
{
	for(auto& it : _bar_generator)
	{
		for(auto p_it : it.second)
		{
			p_it.second->update();
		}
	}
}

const std::vector<bar_info> data_channel::get_kline(const code_t& code, uint32_t period, size_t length)const
{
	auto it = _bar_generator.find(code);
	if(it == _bar_generator.end())
	{
		PRINT_ERROR("cant find subscribe code ",code.to_string());
		return std::vector<bar_info>();
	}
	auto pit = it->second.find(period);
	if (pit == it->second.end())
	{
		PRINT_ERROR("cant find subscribe the code period", code.to_string(), period);
		return std::vector<bar_info>();
	}

	return pit->second->get_kline(length);
}