#pragma once
#include <any>
#include <vector>
#include <iostream>
#include <boost/lockfree/spsc_queue.hpp>

typedef enum event_type ENUM_TYPE_INT
{
	ET_Invalid,
	ET_LoadFinish,
	ET_TickReceived,
	ET_AccountChange,
	ET_PositionChange,
	ET_SettlementCompleted,
	ET_OrderCancel,
	ET_OrderPlace,
	ET_OrderDeal,
	ET_OrderTrade,
	ET_OrderError
} event_type;

struct event_data
{
	event_type type;
	std::vector<std::any> param;
};

template<size_t N>
class event_source
{
private:

	boost::lockfree::spsc_queue<event_data, boost::lockfree::capacity<N>>  _event_queue;
	std::vector<std::function<void(event_type, const std::vector<std::any>& param)>> _handle_list;


private:

	void fire_event(event_data& data)
	{
		while (!_event_queue.push(data));
	}


	template<typename T, typename... Types>
	void fire_event(event_data& data, const T& firstArg, const Types&... args) {
		data.param.emplace_back(firstArg);
		fire_event(data, args...);
	}

protected:

	template<typename... Types>
	void fire_event(event_type type, const Types&... args) {
		event_data data;
		data.type = type;
		fire_event(data, args...);
	}


public:

	void update()
	{
		event_data data;
		while (_event_queue.pop(data))
		{
			for (auto& handle : _handle_list)
			{
				handle(data.type, data.param);
			}
		}
	}


public:

	void add_handle(std::function<void(event_type, const std::vector<std::any>&)> handle)
	{
		_handle_list.emplace_back(handle);
	}


};