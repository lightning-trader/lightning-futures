#include "position_container.h"
#include <data_types.hpp>
#include <log_wapper.hpp>

position_container::position_container()
{
}

position_container::~position_container()
{
}

void position_container::increase_position(const code_t& code, direction_type direction, double_t price, uint32_t volume)
{
	spin_lock lock(_mutex);
	auto& pos = _position_info[code];
	pos.id = code ;
	if(direction == direction_type::DT_LONG)
	{
		pos.today_long.price = (pos.today_long.postion * pos.today_long.price + price * volume)/(pos.today_long.postion + volume);
		pos.today_long.postion += volume;
	}else if(direction == direction_type::DT_SHORT)
	{
		pos.today_short.price = (pos.today_short.postion * pos.today_short.price + price * volume) / (pos.today_short.postion + volume);
		pos.today_short.postion += volume;
	}
}

void position_container::reduce_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today, bool is_reduce_frozen )
{
	spin_lock lock(_mutex);
	auto it = _position_info.find(code);
	if(it == _position_info.end())
	{
		return;
	}
	if(is_today)
	{
		if (direction == direction_type::DT_LONG)
		{
			it->second.today_long.postion -= std::min<uint32_t>(volume, it->second.today_long.postion);
			it->second.today_long.frozen -= std::min<uint32_t>(volume, it->second.today_long.frozen);
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.today_short.postion -= std::min<uint32_t>(volume, it->second.today_short.postion);
			it->second.today_short.frozen -= std::min<uint32_t>(volume, it->second.today_short.frozen);
		}
	}
	else
	{
		if (direction == direction_type::DT_LONG)
		{
			it->second.yestoday_long.postion -= std::min<uint32_t>(volume, it->second.yestoday_long.postion);
			it->second.yestoday_long.frozen -= std::min<uint32_t>(volume, it->second.yestoday_long.frozen);
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.yestoday_short.postion -= std::min<uint32_t>(volume, it->second.yestoday_short.postion);
			it->second.yestoday_short.frozen -= std::min<uint32_t>(volume, it->second.yestoday_short.frozen);
		}
	}
	if(it->second.empty())
	{
		_position_info.erase(it);
	}
}

void position_container::frozen_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today)
{
	spin_lock lock(_mutex);
	auto it = _position_info.find(code);
	if (it == _position_info.end())
	{
		return ;
	}
	if (is_today)
	{
		if (direction == direction_type::DT_LONG)
		{
			it->second.today_long.frozen += std::min<uint32_t>(volume, it->second.today_long.postion);
		}
		else if(direction == direction_type::DT_SHORT)
		{
			it->second.today_short.frozen += std::min<uint32_t>(volume, it->second.today_short.postion);
		}
	}else
	{
		if (direction == direction_type::DT_LONG)
		{
			it->second.yestoday_long.frozen += std::min<uint32_t>(volume, it->second.yestoday_long.postion);
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.yestoday_short.frozen += std::min<uint32_t>(volume, it->second.yestoday_short.postion);
		}
	}
	LOG_INFO("position_container::frozen_position ", code.get_id(), it->second.today_long.postion, it->second.today_long.frozen, it->second.today_short.postion, it->second.today_long.frozen);
}

void position_container::thawing_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today)
{
	spin_lock lock(_mutex);
	auto it = _position_info.find(code);
	if (it == _position_info.end())
	{
		return ;
	}
	if (is_today)
	{
		if (direction == direction_type::DT_LONG)
		{
			it->second.today_long.frozen -= std::min<uint32_t>(volume, it->second.today_long.frozen);
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.today_short.frozen -= std::min<uint32_t>(volume, it->second.today_short.frozen);
		}
	}
	else
	{
		if (direction == direction_type::DT_LONG)
		{
			it->second.yestoday_long.frozen -= std::min<uint32_t>(volume, it->second.yestoday_long.frozen);
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.yestoday_short.frozen -= std::min<uint32_t>(volume, it->second.yestoday_short.frozen);
		}
	}
	LOG_INFO("position_container::thawing_position ", code.get_id(), it->second.today_long.postion, it->second.today_long.frozen, it->second.today_short.postion, it->second.today_short.frozen);
}

position_detail position_container::get_position_info(const code_t& code)const
{
	spin_lock lock(_mutex);
	auto it = _position_info.find(code);
	if (it != _position_info.end())
	{
		return it->second;
	}
	return position_detail(code);
}

void position_container::get_all_position(std::vector<position_detail>& position)const
{
	spin_lock lock(_mutex);
	for(auto&it : _position_info)
	{
		position.emplace_back(it.second);
	}
}

void position_container::clear()
{
	spin_lock lock(_mutex);
	_position_info.clear();
	LOG_INFO("position_container::clear ");
}