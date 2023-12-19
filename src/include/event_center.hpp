#pragma once
#include <any>
#include <vector>
#include <map>
#include "ringbuffer.hpp"

template<typename T>
struct event_data
{
	T type ;
	std::vector<std::any> params;

	event_data() = default ;
};
template<typename T>
class event_dispatch
{

	std::multimap<T,std::function<void(const std::vector<std::any>&)>> _handle_map;

public:
	
	void add_handle(T type,std::function<void(const std::vector<std::any>&)> handle)
	{
		_handle_map.insert(std::make_pair(type, handle));
	}

	void clear_handle()
	{
		_handle_map.clear();
	}

protected:

	void trigger(T type,const std::vector<std::any>& params)
	{
		auto it = _handle_map.equal_range(type);
		while(it.first != it.second)
		{
			it.first->second(params);
			it.first++;
		}
	}
};

template<typename T, size_t N>
class queue_event_source : public event_dispatch<T>
{
private:

	Ringbuffer<event_data<T>, N>  _event_queue;

private:

	void fire_event(event_data<T>& data)
	{
		while (!_event_queue.insert(data));
	}


	template<typename A, typename... Types>
	void fire_event(event_data<T>& data, const A& firstArg, const Types&... args) {
		data.params.emplace_back(firstArg);
		fire_event(data, args...);
	}

public:

	void process()
	{
		event_data<T> data;
		while (_event_queue.remove(data))
		{
			this->trigger(data.type, data.params);
		}
	}


public:


	bool is_empty()const
	{
		return _event_queue.isEmpty();
	}

	bool is_full()const
	{
		return _event_queue.isFull();
	}


	template<typename... Types>
	void fire_event(T type, const Types&... args) {
		event_data<T> data;
		data.type = type;
		fire_event(data, args...);
	}

};

template<typename T>
class direct_event_source : public event_dispatch<T>
{

private:

	void fire_event(event_data<T>& data)
	{
		this->trigger(data.type,data.params);
	}


	template<typename A, typename... Types>
	void fire_event(event_data<T>& data, const A& firstArg, const Types&... args) {
		data.params.emplace_back(firstArg);
		fire_event(data, args...);
	}

public:

	template<typename... Types>
	void fire_event(T type, const Types&... args) {
		event_data<T> data;
		data.type = type;
		fire_event(data, args...);
	}

};