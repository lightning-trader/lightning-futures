#include "delayed_distributor.h"

void delayed_distributor::handle_account(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		const auto& account = std::any_cast<account_info>(param[0]);
		if (on_account)
		{
			on_account(account);
		}
	}

}

void delayed_distributor::handle_position(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		auto position = std::any_cast<position_info>(param[0]);
		if (on_position)
		{
			on_position(position);
		}
	}

}


void delayed_distributor::handle_entrust(const std::vector<std::any>& param)
{
	if (param.size() >= 1)
	{
		order_info order = std::any_cast<order_info>(param[0]);
		if (delayed_event.on_entrust)
		{
			delayed_event.on_entrust(order);
		}
	}
}

void delayed_distributor::handle_deal(const std::vector<std::any>& param)
{
	if (param.size() >= 3)
	{
		estid_t localid = std::any_cast<estid_t>(param[0]);
		uint32_t deal_volume = std::any_cast<uint32_t>(param[1]);
		uint32_t total_volume = std::any_cast<uint32_t>(param[2]);
		if (delayed_event.on_deal)
		{
			delayed_event.on_deal(localid, deal_volume, total_volume);
		}
	}
}

void delayed_distributor::handle_trade(const std::vector<std::any>& param)
{
	if (param.size() >= 6)
	{

		estid_t localid = std::any_cast<estid_t>(param[0]);
		code_t code = std::any_cast<code_t>(param[1]);
		offset_type offset = std::any_cast<offset_type>(param[2]);
		direction_type direction = std::any_cast<direction_type>(param[3]);
		double_t price = std::any_cast<double_t>(param[4]);
		uint32_t trade_volume = std::any_cast<uint32_t>(param[5]);

		if (delayed_event.on_trade)
		{
			delayed_event.on_trade(localid, code, offset, direction, price, trade_volume);
		}
	}
}

void delayed_distributor::handle_cancel(const std::vector<std::any>& param)
{
	if (param.size() >= 7)
	{
		estid_t localid = std::any_cast<estid_t>(param[0]);
		code_t code = std::any_cast<code_t>(param[1]);
		offset_type offset = std::any_cast<offset_type>(param[2]);
		direction_type direction = std::any_cast<direction_type>(param[3]);
		double_t price = std::any_cast<double_t>(param[4]);
		uint32_t cancel_volume = std::any_cast<uint32_t>(param[5]);
		uint32_t total_volume = std::any_cast<uint32_t>(param[6]);
		if (delayed_event.on_cancel)
		{
			delayed_event.on_cancel(localid, code, offset, direction, price, cancel_volume, total_volume);
		}
	}
}
void delayed_distributor::handle_error(const std::vector<std::any>& param)
{
	if (param.size() >= 2)
	{
		const error_type type = std::any_cast<error_type>(param[0]);
		const estid_t localid = std::any_cast<estid_t>(param[1]);
		const uint32_t error_code = std::any_cast<uint32_t>(param[2]);

		if (delayed_event.on_error)
		{
			delayed_event.on_error(type, localid, error_code);
		}
	}
}