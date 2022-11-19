#include "recorder_proxy.h"
#include <data_types.hpp>

recorder_proxy::recorder_proxy(recorder* recorder, uint32_t interval) :_recorder(recorder), _is_reocding(true), _record_thread([this, interval]()->void {
	while (_is_reocding)
	{
		this->record_update();
		std::this_thread::sleep_for(std::chrono::milliseconds(interval));
	}
	})
{}

void recorder_proxy::record_update()
{
	if (_recorder == nullptr)
	{
		return ;
	}
	std::tuple<time_t, order_info> order_entrust_data;
	while (_order_entrust_queue.pop(order_entrust_data))
	{
		_recorder->record_order_entrust(std::get<0>(order_entrust_data), std::get<1>(order_entrust_data));
	}

	std::tuple<time_t, estid_t> order_trade_data;
	while (_order_trade_queue.pop(order_trade_data))
	{
		_recorder->record_order_trade(std::get<0>(order_trade_data), std::get<1>( order_trade_data));
	}

	std::tuple<time_t, estid_t, uint32_t> order_cancel_data;
	while (_order_cancel_queue.pop(order_cancel_data))
	{
		_recorder->record_order_cancel(std::get<0>(order_cancel_data), std::get<1>(order_cancel_data), std::get<2>(order_cancel_data));
	}

	std::tuple<time_t, position_info> position_flow_data;
	while (_position_flow_queue.pop(position_flow_data))
	{
		_recorder->record_position_flow(std::get<0>(position_flow_data), std::get<1>( position_flow_data));
	}

	std::tuple<time_t, account_info> account_flow_data;
	while (_account_flow_queue.pop(account_flow_data))
	{
		_recorder->record_account_flow(std::get<0>(account_flow_data), std::get<1>(account_flow_data));
	}
}

recorder_proxy::~recorder_proxy()
{
	_is_reocding = false ;
	_record_thread.join();
	if (_recorder)
	{
		delete _recorder;
		_recorder = nullptr;
	}
}


void recorder_proxy::record_order_entrust(time_t time, const order_info& order)
{
	while (!_order_entrust_queue.push(std::make_tuple(time, order)));
}

void recorder_proxy::record_order_trade(time_t time, estid_t localid)
{
	while (!_order_trade_queue.push(std::make_tuple(time, localid)));
}
void recorder_proxy::record_order_cancel(time_t time, estid_t localid, uint32_t last_volume)
{
	while (!_order_cancel_queue.push(std::make_tuple(time, localid, last_volume)));
}

void recorder_proxy::record_position_flow(time_t time, const position_info& position)
{
	while (!_position_flow_queue.push(std::make_tuple(time,position)));
}

void recorder_proxy::record_account_flow(time_t time, const account_info& account)
{
	while (!_account_flow_queue.push(std::make_tuple(time, account)));
}