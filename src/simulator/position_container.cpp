#include "position_container.h"
#include <data_types.hpp>

position_container::position_container()
{
}

position_container::~position_container()
{
}

void position_container::increase_position(const code_t& code, direction_type direction, double_t price, uint32_t volume)
{
	spin_lock lock(_mutex);
	auto& pos = _today_position[code];
	pos.id = code ;
	
	if(direction == direction_type::DT_LONG)
	{
		auto& long_price = _avg_long_price[code];
		long_price = (pos.long_cell.volume * long_price + price * volume)/(pos.long_cell.volume + volume);
		pos.long_cell.volume += volume;
		_avg_long_price[code] = long_price;
	}else if(direction == direction_type::DT_SHORT)
	{
		auto& short_price = _avg_short_price[code];
		short_price = (pos.short_cell.volume * short_price + price * volume) / (pos.short_cell.volume + volume);
		pos.short_cell.volume += volume;
		_avg_short_price[code] = short_price;
	}
}

void position_container::reduce_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today, bool is_reduce_frozen )
{
	spin_lock lock(_mutex);

	if(is_today)
	{
		auto it = _today_position.find(code);
		if (it == _today_position.end())
		{
			return;
		}
		if (direction == direction_type::DT_LONG)
		{
			it->second.long_cell.volume -= std::min<uint32_t>(volume, it->second.long_cell.volume);
			if(is_reduce_frozen)
			{
				it->second.long_cell.frozen -= std::min<uint32_t>(volume, it->second.long_cell.frozen);
			}
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.short_cell.volume -= std::min<uint32_t>(volume, it->second.short_cell.volume);
			if (is_reduce_frozen)
			{
				it->second.short_cell.frozen -= std::min<uint32_t>(volume, it->second.short_cell.frozen);
			}
		}
		if (it->second.empty())
		{
			_today_position.erase(it);
		}
	}
	else
	{
		auto it = _history_position.find(code);
		if (it == _history_position.end())
		{
			return;
		}
		if (direction == direction_type::DT_LONG)
		{
			it->second.long_cell.volume -= std::min<uint32_t>(volume, it->second.long_cell.volume);
			if (is_reduce_frozen)
			{
				it->second.long_cell.frozen -= std::min<uint32_t>(volume, it->second.long_cell.frozen);
			}
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.short_cell.volume -= std::min<uint32_t>(volume, it->second.short_cell.volume);
			if (is_reduce_frozen)
			{
				it->second.short_cell.frozen -= std::min<uint32_t>(volume, it->second.short_cell.frozen);
			}
		}
		if (it->second.empty())
		{
			_history_position.erase(it);
		}
	}
	
}

void position_container::frozen_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today)
{
	spin_lock lock(_mutex);

	if (is_today)
	{
		auto it = _today_position.find(code);
		if (it == _today_position.end())
		{
			return;
		}
		if (direction == direction_type::DT_LONG)
		{
			it->second.long_cell.frozen += std::min<uint32_t>(volume, it->second.long_cell.volume);
		}
		else if(direction == direction_type::DT_SHORT)
		{
			it->second.short_cell.frozen += std::min<uint32_t>(volume, it->second.short_cell.volume);
		}
		LOG_INFO("position_container::frozen_position _today_position %s %d %d %d %d", code.get_id(), it->second.long_cell.volume, it->second.long_cell.frozen, it->second.short_cell.volume, it->second.short_cell.frozen);
	}
	else
	{
		auto it = _history_position.find(code);
		if (it == _history_position.end())
		{
			return;
		}
		if (direction == direction_type::DT_LONG)
		{
			it->second.long_cell.frozen += std::min<uint32_t>(volume, it->second.long_cell.volume);
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.short_cell.frozen += std::min<uint32_t>(volume, it->second.short_cell.volume);
		}
		LOG_INFO("position_container::frozen_position _history_position %s %d %d %d %d", code.get_id(), it->second.long_cell.volume, it->second.long_cell.frozen, it->second.short_cell.volume, it->second.short_cell.frozen);
	}
}

void position_container::thawing_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today)
{
	spin_lock lock(_mutex);

	if (is_today)
	{
		auto it = _today_position.find(code);
		if (it == _today_position.end())
		{
			return;
		}
		if (direction == direction_type::DT_LONG)
		{
			it->second.long_cell.frozen -= std::min<uint32_t>(volume, it->second.long_cell.frozen);
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.short_cell.frozen -= std::min<uint32_t>(volume, it->second.short_cell.frozen);
		}
		LOG_INFO("position_container::thawing_position _today_position %s %d %d %d %d", code.get_id(), it->second.long_cell.volume, it->second.long_cell.frozen, it->second.short_cell.volume, it->second.short_cell.frozen);
	}
	else
	{
		auto it = _history_position.find(code);
		if (it == _history_position.end())
		{
			return;
		}
		if (direction == direction_type::DT_LONG)
		{
			it->second.long_cell.frozen -= std::min<uint32_t>(volume, it->second.long_cell.frozen);
		}
		else if (direction == direction_type::DT_SHORT)
		{
			it->second.short_cell.frozen -= std::min<uint32_t>(volume, it->second.short_cell.frozen);
		}
		LOG_INFO("position_container::thawing_position _history_position %s %d %d %d %d", code.get_id(), it->second.long_cell.volume, it->second.long_cell.frozen, it->second.short_cell.volume, it->second.short_cell.frozen);
	}
}

void position_container::crossday_move_position(std::function<void(const code_t&)> move_callback)
{
	for (const auto& it : _today_position)
	{
		//只有上期所区分昨仓今仓
		if (it.first.is_split_position())
		{
			_history_position[it.first].id = it.second.id;
			_history_position[it.first].long_cell.volume += it.second.long_cell.volume;
			_history_position[it.first].long_cell.frozen += it.second.long_cell.frozen;
			_history_position[it.first].short_cell.volume += it.second.short_cell.volume;
			_history_position[it.first].short_cell.frozen += it.second.short_cell.frozen;
			if (move_callback)
			{
				move_callback(it.second.id);
			}
		}
	}
}

position_info position_container::get_merge_position(const code_t& code)const
{
	spin_lock lock(_mutex);
	auto it = _today_position.find(code);
	if (it == _today_position.end())
	{
		auto yit = _history_position.find(code);
		if (yit == _history_position.end())
		{
			return position_info(code);
		}
		return yit->second;
	}
	auto yit = _history_position.find(code);
	if (yit == _history_position.end())
	{
		return it->second;
	}
	position_info result = it->second;
	result.long_cell.volume += yit->second.long_cell.volume;
	result.long_cell.frozen += yit->second.long_cell.frozen;
	result.short_cell.volume += yit->second.short_cell.volume;
	result.short_cell.frozen += yit->second.short_cell.frozen;
	return result;
}

position_info position_container::get_today_position(const code_t& code)const 
{
	spin_lock lock(_mutex);
	auto it = _today_position.find(code);
	if (it != _today_position.end())
	{
		return it->second;
	}
	return position_info(code);
}

position_info position_container::get_history_position(const code_t& code)const 
{
	spin_lock lock(_mutex);
	auto it = _history_position.find(code);
	if (it != _history_position.end())
	{
		return it->second;
	}
	return position_info(code);
}

double_t position_container::get_avg_price(const code_t& code, direction_type direction)const
{
	if (direction == direction_type::DT_LONG)
	{
		auto it = _avg_long_price.find(code);
		if (it == _avg_long_price.end())
		{
			return .0F;
		}
		return it->second;
	}
	else 
	{
		auto it = _avg_short_price.find(code);
		if (it == _avg_short_price.end())
		{
			return .0F;
		}
		return it->second;
	}
}

void position_container::get_all_position(std::map<code_t,position_info>& all_position)const
{
	spin_lock lock(_mutex);
	all_position = _today_position;
	for(auto&it : _history_position)
	{
		all_position[it.first].id = it.second.id;
		all_position[it.first].long_cell.volume += it.second.long_cell.volume;
		all_position[it.first].long_cell.frozen += it.second.long_cell.frozen;
		all_position[it.first].short_cell.volume += it.second.short_cell.volume;
		all_position[it.first].short_cell.frozen += it.second.short_cell.frozen;
	}
}

void position_container::clear()
{
	spin_lock lock(_mutex);
	_today_position.clear();
	_history_position.clear();
	_avg_long_price.clear();
	_avg_short_price.clear();
	LOG_INFO("position_container::clear ");
}

uint32_t position_container::get_history_usable(const code_t& code,direction_type direction)const
{
	auto it = _history_position.find(code);
	if (it == _history_position.end())
	{
		return 0;
	}
	if (direction == direction_type::DT_LONG)
	{
		return it->second.long_cell.usable();
	}
	else 
	{
		return it->second.short_cell.usable();
	}
}