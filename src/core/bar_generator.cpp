#include "bar_generator.h"
#include "time_utils.hpp"

void bar_generator::insert_tick(const tick_info& tick)
{
	uint32_t delta_volume = (tick.volume - _prev_volume);
	if(_bar.open == .0F)
	{
		_poc_data.clear();
		_bar.open = tick.price;
		_bar.time = _minute * ONE_MINUTE_SECONDS;
		_poc_data[tick.price] = delta_volume;
		_bar.poc = tick.price;
	}
	_bar.high = std::max(_bar.high, tick.price);
	_bar.low = std::min(_bar.low, tick.price);
	_bar.close = tick.price;
	_bar.volume += delta_volume;
	_poc_data[tick.price] += delta_volume;
	if(_poc_data[tick.price]> _poc_data[_bar.poc])
	{
		_bar.poc = tick.price;
	}
	if(tick.price == tick.buy_price())
	{
		//主动卖出
		_bar.price_sell_volume[tick.price] += delta_volume;
	}
	if (tick.price == tick.sell_price())
	{
		//主动买出
		_bar.price_buy_volume[tick.price] += delta_volume;
	}
	if(tick.time / ONE_MINUTE_SECONDS - _minute >= _period)
	{
		//合成
		_bar.close = tick.price;
		if(_bar_finish)
		{
			_bar_finish(_bar);
		}
		_minute = static_cast<uint32_t>(tick.time / ONE_MINUTE_SECONDS);
		_bar.open = .0F;
		_bar.high = .0F;
		_bar.low = .0F;
		_bar.close = .0F;
		_bar.time = 0;
	}
	_prev_volume = tick.volume;
}