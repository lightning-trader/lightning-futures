#include "prb_signal.h"
#include "data_types.hpp"


void prb_signal::on_tick(const tick_info& tick, const deal_info& deal)
{
	_price_volume[tick.price] += deal.volume_delta;
	if(_price_volume[_max_volume_key] < _price_volume[tick.price])
	{
		_max_volume_key = tick.price;
	}
	
	if(tick.price > _delta)
	{
		if(_trigger)
		{
			_trigger(ST_BUY);
		}
	}
	if(tick.price < _delta)
	{
		if (_trigger)
		{
			_trigger(ST_SELL);
		}
	}
}

