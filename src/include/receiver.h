#pragma once
#include <define.h>

namespace lt
{
	struct tick_receiver
	{
		virtual void on_tick(const tick_info& tick, const deal_info& deal) = 0;
	};

	struct bar_receiver
	{
		virtual void on_bar(uint32_t period,const bar_info& bar) = 0;
	};
}

