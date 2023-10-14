#pragma once
#include <any>
#include <vector>
#include "ringbuffer.hpp"

template<typename T>
struct event_data
{
	T type;
	std::vector<std::any> params;
};
template<typename T>
class event_dispatch
{

	std::vector<std::function<void(T, const std::vector<std::any>& param)>> _handle_list;

public:
	
	void add_handle(std::function<void(T, const std::vector<std::any>&)> handle)
	{
		_handle_list.emplace_back(handle);
	}

protected:

	void trigger(T type,const std::vector<std::any>& params)
	{
		for (auto& handle : _handle_list)
		{
			handle(type, params);
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
			trigger(data.type, data.params);
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
	
	std::vector<std::function<void(T, const std::vector<std::any>& param)>> _handle_list;

private:

	void fire_event(event_data<T>& data)
	{
		trigger(data.type,data.params);
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