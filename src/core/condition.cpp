#pragma once
#include "condition.h"

bool time_out_cdt::check(const tick_info* tick)const
{
	if (tick)
	{
		if (tick->time > _delay_seconds)
		{
			return true;
		}
	}
	return false;
}
bool price_up_cdt::check(const tick_info* tick)const
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
bool price_down_cdt::check(const tick_info* tick)const
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