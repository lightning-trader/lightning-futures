#pragma once
#include <any>
#include <vector>
#include <iostream>
#include <boost/lockfree/spsc_queue.hpp>

template<typename T>
struct event_data
{
	T type;
	std::vector<std::any> param;
};

template<typename T,size_t N>
class event_source
{
private:

	boost::lockfree::spsc_queue<event_data<T>, boost::lockfree::capacity<N>>  _event_queue;
	std::vector<std::function<void(T, const std::vector<std::any>& param)>> _handle_list;


private:

	void fire_event(event_data<T>& data)
	{
		while (!_event_queue.push(data));
	}


	template<typename A, typename... Types>
	void fire_event(event_data<T>& data, const A& firstArg, const Types&... args) {
		data.param.emplace_back(firstArg);
		fire_event(data, args...);
	}

protected:

	template<typename... Types>
	void fire_event(T type, const Types&... args) {
		event_data<T> data;
		data.type = type;
		fire_event(data, args...);
	}


public:

	void update()
	{
		event_data<T> data;
		while (_event_queue.pop(data))
		{
			for (auto& handle : _handle_list)
			{
				handle(data.type, data.param);
			}
		}
	}

	bool is_empty()const
	{
		return _event_queue.read_available() == 0;
	}

	bool is_full()const
	{
		return _event_queue.write_available() == 0;
	}

public:

	void add_handle(std::function<void(T, const std::vector<std::any>&)> handle)
	{
		_handle_list.emplace_back(handle);
	}


};