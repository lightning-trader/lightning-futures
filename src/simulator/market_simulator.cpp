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
#include "market_simulator.h"
#include <event_center.hpp>
#include <thread>
#include <csv_tick_loader.h>
#include <ltds_tick_loader.h>
#include <log_define.hpp>

using namespace lt;
using namespace lt::driver;

market_simulator::market_simulator(const params& config) :_loader(nullptr),
_current_day_index(0U),
_current_time(0),
_current_index(0),
_interval(1),
_is_finished(false),
_is_runing(false)
{
	_interval = config.get<uint32_t>("interval");
	const auto & loader_type = config.get<std::string>("loader_type");
	if (loader_type == "ltds")
	{
		const auto& channel = config.get<std::string>("channel");
		const auto& cache_path = config.get<std::string>("cache_path");
		const auto& lru_size = config.get<size_t>("lru_size");
		_loader = new ldts_tick_loader(channel, cache_path, lru_size);
	}
	else if (loader_type == "csv")
	{
		const auto& data_path = config.get<std::string>("csv_data_path");
		const auto& index_file = config.get<std::string>("trading_day_file");
		_loader = new csv_tick_loader(data_path.c_str(), index_file.c_str());

	}
}
market_simulator::~market_simulator()
{
	if (_loader)
	{
		delete _loader;
		_loader = nullptr;
	}
}
void market_simulator::set_trading_range(uint32_t begin, uint32_t end)
{
	if(_loader)
	{
		_loader->load_trading_day(_all_trading_day,begin,end);
	}
}

void market_simulator::set_publish_callback(std::function<void(const std::vector<const lt::tick_info*>&)> publish_callback)
{
	_publish_callback = publish_callback;
}

void market_simulator::set_crossday_callback(std::function<void(uint32_t form, uint32_t to)> crossday_callback)
{
	_crossday_callback = crossday_callback;
}

void market_simulator::set_finish_callback(std::function<void()> finish_callback)
{
	_finish_callback = finish_callback;
}
bool market_simulator::play()
{
	if(_all_trading_day.empty())
	{
		return false;
	}
	_current_day_index = 0U;
	_current_time = 0;
	_current_index = 0;
	_pending_tick_info.clear();
	_instrument_id_list.clear();
	_is_finished.exchange(false) ;
	_is_runing.exchange(true);
	return true;
}

void market_simulator::pause()
{
	_is_runing.exchange(false);
}

void market_simulator::resume()
{
	_is_runing.exchange(true);
}

bool market_simulator::is_finished() const
{
	return 	_is_finished;
}

void market_simulator::subscribe(const std::set<code_t>& codes)
{
	for(auto& it : codes)
	{
		_instrument_id_list.insert(it);
	}
}

void market_simulator::unsubscribe(const std::set<code_t>& codes)
{
	for (auto& it : codes)
	{
		auto cur = _instrument_id_list.find(it);
		if(cur != _instrument_id_list.end())
		{
			_instrument_id_list.erase(cur);
		}
	}
}

void market_simulator::update()
{
	if(_is_runing)
	{
		publish_tick();
	}
}


void market_simulator::load_data()
{
	if(_loader)
	{
		for(auto& it : _instrument_id_list)
		{
			_loader->load_tick(_pending_tick_info, it, _all_trading_day[_current_day_index]);
		}
		std::sort(_pending_tick_info.begin(), _pending_tick_info.end(), [](const auto& lh, const auto& rh)->bool {

			if (lh.time < rh.time)
			{
				return true;
			}
			if (lh.time > rh.time)
			{
				return false;
			}
			return lh.id < rh.id;
			});
	}
}

void market_simulator::publish_tick()
{	
	if(_pending_tick_info.empty())
	{
		load_data();
	}
	if (_current_index >= _pending_tick_info.size())
	{
		return;
	}
	const tick_detail* tick = &(_pending_tick_info[_current_index]);
	_current_time = tick->time;
	std::vector<const tick_detail*> current_tick;
	
	while(_current_time == tick->time)
	{
		current_tick.emplace_back(tick);
		_current_index++;
		if(_current_index < _pending_tick_info.size())
		{
			tick = &(_pending_tick_info[_current_index]);
		}
		else
		{
			//结束了触发收盘事件
			break;
		}
	}

	if (_publish_callback)
	{
		std::vector<const tick_info*> tick_info;
		for (auto tick : current_tick)
		{
			tick_info.emplace_back(tick);
		}
		_publish_callback(tick_info);
	}

	for(auto tick : current_tick)
	{
		PROFILE_INFO(tick->id.get_symbol());
		fire_event(market_event_type::MET_TickReceived, *static_cast<const tick_info*>(tick),tick->extend);
	}

	if (_current_index >= _pending_tick_info.size())
	{
		finish_publish();
	}
}

void market_simulator::finish_publish()
{
	_current_time = 0;
	_current_index = 0;
	_pending_tick_info.clear();
	_instrument_id_list.clear();
	if(_current_day_index ==_all_trading_day.size()-1)
	{
		_is_finished.exchange(true);
		_is_runing.exchange(false);
		if(_finish_callback)
		{
			_finish_callback();
		}
	}else
	{
		_current_day_index++;
	}
}