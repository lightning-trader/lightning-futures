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
	auto it = _dc->_tick_receiver.find(code);
	if (it == _dc->_tick_receiver.end())
	{
		_dc->_tick_receiver[code].insert(receiver);
	}
	else
	{
		it->second.insert(receiver);
	}
	_dc->_tick_reference_count[code]++;
}

void unsubscriber::unregist_tick_receiver(const code_t& code, tick_receiver* receiver)
{
	auto it = _dc->_tick_receiver.find(code);
	if (it == _dc->_tick_receiver.end())
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
		_dc->_tick_receiver.erase(it);
		auto cit = _dc->_tick_cache.find(code);
		if (cit != _dc->_tick_cache.end()) 
		{
			_dc->_tick_cache.erase(cit);
		}
	}
	auto d_it = _dc->_tick_reference_count.find(code);
	if (d_it != _dc->_tick_reference_count.end())
	{
		if (d_it->second > 0)
		{
			d_it->second--;
		}
	}
}

void subscriber::regist_tape_receiver(const code_t& code, tape_receiver* receiver)
{
	auto it = _dc->_tape_receiver.find(code);
	if (it == _dc->_tape_receiver.end())
	{
		_dc->_tape_receiver[code].insert(receiver);
	}
	else
	{
		it->second.insert(receiver);
	}
	_dc->_tick_reference_count[code]++;
}

void unsubscriber::unregist_tape_receiver(const code_t& code, tape_receiver* receiver)
{
	auto it = _dc->_tape_receiver.find(code);
	if (it == _dc->_tape_receiver.end())
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
		_dc->_tape_receiver.erase(it);
	}
	auto d_it = _dc->_tick_reference_count.find(code);
	if (d_it != _dc->_tick_reference_count.end())
	{
		if (d_it->second > 1)
		{
			d_it->second--;
		}
	}
}

void subscriber::regist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver)
{
	auto generator_iter = _dc->_bar_generator[code].find(period);
	if (generator_iter == _dc->_bar_generator[code].end())
	{
		_dc->_bar_generator[code][period] = std::make_shared<bar_generator>(code,period, _dc->_ctx,_dc->_dw);
	}
	_dc->_bar_generator[code][period]->add_receiver(receiver);
	_dc->_tick_reference_count[code]++;
}

void subscriber::subscribe()
{
	_dc->subscribe();
}

void unsubscriber::unregist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver)
{
	auto it = _dc->_bar_generator.find(code);
	if (it == _dc->_bar_generator.end())
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
			_dc->_bar_generator.erase(it);
		}
	}

	auto d_it = _dc->_tick_reference_count.find(code);
	if (d_it != _dc->_tick_reference_count.end())
	{
		if (d_it->second > 1)
		{
			d_it->second--;
		}
	}
}

void unsubscriber::unsubscribe()
{
	_dc->unsubscribe();
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
		auto tk_it = _tick_receiver.find(tick.code);
		if (tk_it != _tick_receiver.end())
		{
			for (auto tkrc : tk_it->second)
			{
				if (tkrc)
				{
					PROFILE_DEBUG(tick.code.get_symbol());
					auto cit = _tick_cache.find(tick.code);
					if (cit != _tick_cache.end())
					{
						if(cit->second.empty()|| cit->second.back().time<tick.time)
							cit->second.emplace_back(tick);
					}
					tkrc->on_tick(tick);
					PROFILE_DEBUG(tick.code.get_symbol());
					
				}
			}
		}

		auto tp_it = _tape_receiver.find(tick.code);
		if (tp_it != _tape_receiver.end())
		{
			for (auto tprc : tp_it->second)
			{
				if (tprc)
				{
					tape_info deal_info(tick.code, tick.time, tick.price);
					const auto& prev_tick = _ctx->get_previous_tick(tick.code);
					deal_info.volume_delta = static_cast<uint32_t>(tick.volume - prev_tick.volume);
					deal_info.interest_delta = tick.open_interest - prev_tick.open_interest;
					deal_info.direction = get_deal_direction(prev_tick, tick);
					tprc->on_tape(deal_info);
				}
			}
		}

		auto br_it = _bar_generator.find(tick.code);
		if (br_it != _bar_generator.end())
		{
			for (auto bg_it : br_it->second)
			{
				bg_it.second->insert_tick(tick);
			}
		}
	});
	PRINT_INFO("start load history ticks");
	//加载历史tick
	for (const auto& code : tick_subscrib)
	{
		std::vector<ltd_tick_info> preload_ticks;
		auto ret = _dw.get_history_tick(preload_ticks, code.to_string().c_str(), _ctx->get_trading_day());
		if (ret != ltd_error_code::EC_NO_ERROR)
		{
			PRINT_ERROR("get_history_tick error:",static_cast<uint32_t>(ret));
		}
		std::vector<tick_info> ticks;
		for (const auto& it : preload_ticks)
		{
			//
			price_volume_array bid_order;
			for (size_t i = 0; i < WAITING_PRICE_LENGTH && i < PRICE_VOLUME_SIZE; i++)
			{
				bid_order[i] = std::make_pair(it.bid_order[i].price, it.bid_order[i].volume);
			}
			price_volume_array ask_order;
			for (size_t i = 0; i < WAITING_PRICE_LENGTH && i < PRICE_VOLUME_SIZE; i++)
			{
				ask_order[i] = std::make_pair(it.ask_order[i].price, it.ask_order[i].volume);
			}

			ticks.emplace_back(tick_info(code_t(it.code), it.time, it.price, it.volume, it.open_interest, it.average_price, it.trading_day, std::move(bid_order), std::move(ask_order)));
		}
		_tick_cache.insert(std::make_pair(code, ticks));
	}
	PRINT_INFO("start load history kline");
	//加载历史k线
	for (const auto& generator_pair : _bar_generator)
	{
		for (const auto& piroed_pair:generator_pair.second)
		{
			piroed_pair.second->load_history();
		}
	}
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
	_tick_cache.clear();
	//加载历史k线
	for (const auto& generator_pair : _bar_generator)
	{
		for (const auto& piroed_pair : generator_pair.second)
		{
			piroed_pair.second->clear_history();
		}
	}
}

bool data_channel::polling()
{
	bool result = false ;
	for(auto& it : _bar_generator)
	{
		for(auto p_it : it.second)
		{
			result |= p_it.second->polling();
		}
	}
	return result;
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

const std::vector<tick_info> data_channel::get_ticks(const code_t& code, size_t length) const
{
	auto it = _tick_cache.find(code);
	if (it == _tick_cache.end())
	{
		PRINT_ERROR("cant find the code tick data ", code.to_string());
		return std::vector<tick_info>();
	}
	std::vector<tick_info> last_ticks(it->second.end() - length, it->second.end());
	return last_ticks;
}