#include "market_simulator.h"
#include <data_types.hpp>
#include <event_center.hpp>
#include <thread>
#include "./tick_loader/csv_tick_loader.h"

bool market_simulator::init(const boost::property_tree::ptree& config)
{
	std::string loader_type ;
	std::string csv_data_path ;
	try
	{
		_interval = config.get<uint32_t>("interval",1);
		loader_type = config.get<std::string>("loader_type");
		csv_data_path = config.get<std::string>("csv_data_path");
	}
	catch (...)
	{
		LOG_ERROR("tick_simulator init error ");
		return false;
	}
	if (loader_type == "csv")
	{
		csv_tick_loader* loader = new csv_tick_loader();
		if(!loader->init(csv_data_path))
		{
			delete loader;
			return false ;
		}
		_loader = loader;
	}
	return true;
}

void market_simulator::play(uint32_t tradeing_day, std::function<void(const tick_info& info)> publish_callback)
{
	_current_time = 0;
	_current_tick = 0;
	_current_index = 0;
	_pending_tick_info.clear();

	for (auto& it : _instrument_id_list)
	{
		load_data(it, tradeing_day);
	}

	_is_runing = true ;
	while (_is_runing)
	{
		auto begin = std::chrono::system_clock::now();
		//先触发tick，再进行撮合
		publish_tick(publish_callback);
		auto use_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin);
		auto duration = std::chrono::microseconds(_interval);
		if (use_time < duration)
		{
			std::this_thread::sleep_for(duration - use_time);
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::microseconds(0));
		}
	}
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


void market_simulator::load_data(const code_t& code, uint32_t trading_day)
{
	if(_loader)
	{
		_loader->load_tick(_pending_tick_info,code, trading_day);
	}
}

void market_simulator::publish_tick(std::function<void(const tick_info& info)> publish_callback)
{	
	const tick_info* tick = nullptr;
	if (_current_index < _pending_tick_info.size())
	{
		tick = &(_pending_tick_info[_current_index]);
		_current_time = tick->time;
		_current_tick = tick->tick;
	}
	else
	{
		//结束了触发收盘事件
		_is_runing = false;
		return;
	}
	while(_current_time == tick->time && _current_tick == tick->tick)
	{
		if(tick->trading_day != _current_trading_day)
		{
			_current_trading_day = tick->trading_day;
		}
		this->fire_event(market_event_type::MET_TickReceived, *tick);
		if(publish_callback)
		{
			publish_callback(*tick);
		}
		_current_index++;
		if(_current_index < _pending_tick_info.size())
		{
			tick = &(_pending_tick_info[_current_index]);
			
		}
		else
		{
			//结束了触发收盘事件
			_is_runing = false ;
			break;
		}
	}
}
