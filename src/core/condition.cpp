#pragma once
#include "condition.h"

bool time_out_cds::check(const tick_info* tick)
{
	if (tick)
	{
		if (tick->time > _target_time)
		{
			return true;
		}
	}
	return false;
}
bool price_up_cds::check(const tick_info* tick)
{
	if (tick)
	{
		if (tick->price > _target_price)
		{
			return true;
		}
	}
	return false;
}
bool price_down_cds::check(const tick_info* tick)
{
	if (tick)
	{
		if (tick->price < _target_price)
		{
			return true;
		}
	}
	return false;
}

bool fall_back_cds::check(const tick_info* tick)
{
	if (tick)
	{
		if (tick->price > _highest_price)
		{
			_highest_price = tick->price;
		}
		if(tick->price < _highest_price - _fall_offset)
		{
			return true ;
		}
	}
	return false;
}

bool bounce_back_cds::check(const tick_info* tick)
{
	if (tick)
	{
		if (tick->price < _lowest_price)
		{
			_lowest_price = tick->price;
		}
		if(tick->price > _lowest_price + _bounce_offset)
		{
			return true ;
		}
	}
	return false;
}