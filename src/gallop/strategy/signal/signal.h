#pragma once
#include <define.h>

enum signal_type
{
	ST_INVALID,
	ST_BUY,
	ST_SELL,
};

typedef std::function<void(signal_type code)> signal_function;


class signal 
{
	
protected:

	signal_function _trigger ;

public:

	signal(signal_function tregger):_trigger(tregger) {}

	void tick(const tick_info& tick, const deal_info& deal)
	{
		on_tick(tick, deal);
	}

protected:
	/*
	 *	tick推送
	 */
	virtual void on_tick(const tick_info& tick, const deal_info& deal) = 0;

	/*
	 *	tick k线合成
	 */
	//virtual void on_bar(const tick_info& tick, const deal_info& deal) = 0;

};

