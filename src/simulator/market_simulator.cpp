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
#include "tick_loader/csv_tick_loader.h"
#include <log_wapper.hpp>

using namespace lt;
using namespace lt::driver;

market_simulator::market_simulator(const params& config) :_loader(nullptr),
_current_trading_day(0),
_current_time(0),
_current_index(0),
_interval(1),
_is_finished(false),
_state(execute_state::ES_Idle)
{
	std::string loader_type;
	std::string csv_data_path;
	try
	{
		_interval = config.get<uint32_t>("interval");
		loader_type = config.get<std::string>("loader_type");
		csv_data_path = config.get<std::string>("csv_data_path");
	}
	catch (...)
	{
		LOG_ERROR("tick_simulator init error ");
	}
	if (loader_type == "csv")
	{
		csv_tick_loader* loader = new csv_tick_loader();
		if (!loader->init(csv_data_path))
		{
			delete loader;
		}
		else
		{
			_loader = loader;
		}
		
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

void market_simulator::play(uint32_t trading_day, std::function<void(const std::vector<const tick_info*>&)> publish_callback)
{
	_current_trading_day = trading_day;
	_is_finished = false ;
	_publish_callback = publish_callback;
	_state = execute_state::ES_LoadingData;
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
	switch(_state)
	{
		case execute_state::ES_LoadingData:
			load_data();
		break;
		case execute_state::ES_PublishTick:
			publish_tick();
		break;
	}
}


void market_simulator::load_data()
{
	if(_loader)
	{
		for(auto& it : _instrument_id_list)
		{
			_loader->load_tick(_pending_tick_info, it, _current_trading_day);
		}
		_state = execute_state::ES_PublishTick;
	}
}

void market_simulator::publish_tick()
{	
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
		PROFILE_INFO(tick->id.get_id());
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
	_is_finished = true;
	_state = execute_state::ES_Idle;
}