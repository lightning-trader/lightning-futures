#include "bar_generator.h"
#include "time_utils.hpp"

using namespace lt ;

void bar_generator::insert_tick(const tick_info& tick)
{
	if (tick.time / ONE_MINUTE_MILLISECONDS - _minute >= _period)
	{
		//合成
		if (_minute > 0)
		{
			for (auto it : _bar_callback)
			{
				it->on_bar(_bar);
			}
		}
		_minute = (tick.time / ONE_MINUTE_MILLISECONDS);
		_bar.clear();
	}
	uint32_t delta_volume = static_cast<uint32_t>(tick.volume - _prev_volume);
	if(_bar.open == .0F)
	{
		_poc_data.clear();
		_bar.id = tick.id;
		_bar.period = _period;
		_bar.open = tick.price;
		_bar.close = tick.price;
		_bar.high = tick.price;
		_bar.low = tick.price;
		_bar.time = _minute * ONE_MINUTE_MILLISECONDS;
		_bar.volume = delta_volume;
		_bar.price_step = _price_step ;
		_poc_data[tick.price] = delta_volume;
		_bar.poc = tick.price;
	}
	else 
	{
		_bar.high = std::max(_bar.high, tick.price);
		_bar.low = std::min(_bar.low, tick.price);
		_bar.close = tick.price;
		_bar.volume += delta_volume;
		_poc_data[tick.price] += delta_volume;
	}

	if(_poc_data[tick.price]> _poc_data[_bar.poc])
	{
		_bar.poc = tick.price;
	}
	if(tick.price == tick.buy_price())
	{
		//主动卖出
		_bar.price_sell_volume[tick.price] += delta_volume;
		_bar.delta -= delta_volume;
	}
	if (tick.price == tick.sell_price())
	{
		//主动买出
		_bar.price_buy_volume[tick.price] += delta_volume;
		_bar.delta += delta_volume;
	}

	_prev_volume = tick.volume;
}

void bar_generator::add_receiver(bar_receiver * receiver)
{
	auto it = _bar_callback.find(receiver);
	if (it == _bar_callback.end())
	{
		_bar_callback.insert(receiver);
	}
}

void bar_generator::remove_receiver(bar_receiver* receiver)
{
	auto it = _bar_callback.find(receiver);
	if(it != _bar_callback.end())
	{
		_bar_callback.erase(receiver);
	}
}

bool bar_generator::invalid()const
{
	return _bar_callback.empty();
}