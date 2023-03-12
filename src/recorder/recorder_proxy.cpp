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
	std::pair<order_action_type, std::any> order_action_data;
	while (_order_lifecycle_queue.pop(order_action_data))
	{
		switch(order_action_data.first)
		{
			case OAT_ENTRUST:
			{
				auto order_entrust_data = std::any_cast<std::tuple<time_t, order_info>>(order_action_data.second);
				_recorder->record_order_entrust(std::get<0>(order_entrust_data), std::get<1>(order_entrust_data));
			}
			break;
			case OAT_TRADE:
			{
				auto order_trade_data = std::any_cast<std::tuple<time_t, estid_t>>(order_action_data.second);
				_recorder->record_order_trade(std::get<0>(order_trade_data), std::get<1>(order_trade_data));
			}
			break;
			case OAT_CANCEL:
			{
				auto order_cancel_data = std::any_cast<std::tuple<time_t, estid_t, uint32_t>>(order_action_data.second);
				_recorder->record_order_cancel(std::get<0>(order_cancel_data), std::get<1>(order_cancel_data), std::get<2>(order_cancel_data));
			}
			break;

		}
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

	std::tuple<time_t, uint32_t, order_statistic, account_info > crossday_flow_data;
	while (_crossday_flow_queue.pop(crossday_flow_data))
	{
		_recorder->record_crossday_flow(std::get<0>(crossday_flow_data), std::get<1>(crossday_flow_data), std::get<2>(crossday_flow_data), std::get<3>(crossday_flow_data));
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
	LOG_DEBUG("recorder_proxy record_order_entrust %d %llu", time, order.est_id);
	std::tuple<time_t, order_info> order_action_data = std::make_tuple(time, order);
	std::pair<order_action_type, std::any> order_lifecycle_data = std::make_pair(OAT_ENTRUST, order_action_data);
	while (!_order_lifecycle_queue.push(order_lifecycle_data));
}

void recorder_proxy::record_order_trade(time_t time, estid_t localid)
{
	LOG_DEBUG("recorder_proxy record_order_trade %d %llu", time, localid);
	std::tuple<time_t, estid_t> order_action_data = std::make_tuple(time, localid);
	std::pair<order_action_type, std::any> order_lifecycle_data = std::make_pair(OAT_TRADE, order_action_data);
	while (!_order_lifecycle_queue.push(order_lifecycle_data));
}
void recorder_proxy::record_order_cancel(time_t time, estid_t localid, uint32_t last_volume)
{
	LOG_DEBUG("recorder_proxy record_order_cancel %d %llu", time, localid);
	std::tuple<time_t, estid_t, uint32_t> order_action_data = std::make_tuple(time, localid, last_volume);
	std::pair<order_action_type, std::any> order_lifecycle_data = std::make_pair(OAT_CANCEL, order_action_data);
	while (!_order_lifecycle_queue.push(order_lifecycle_data));
}

void recorder_proxy::record_position_flow(time_t time, const position_info& position)
{
	while (!_position_flow_queue.push(std::make_tuple(time,position)));
}

void recorder_proxy::record_account_flow(time_t time, const account_info& account)
{
	while (!_account_flow_queue.push(std::make_tuple(time, account)));
}

void recorder_proxy::record_crossday_flow(time_t time, uint32_t trading_day, const order_statistic& statistic, const account_info& account)
{
	while (!_crossday_flow_queue.push(std::make_tuple(time, trading_day, statistic, account)));
}