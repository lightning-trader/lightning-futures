#pragma once
#include "signal.h"
#include "define.h"

class prb_signal : public signal
{

public:
	
	prb_signal(uint32_t delta, signal_function trigger):
		signal(trigger),
		_delta(delta),
		_max_volume_key(.0F)
		{
		};


	~prb_signal(){};


public:


	/*
	 *	tickÍÆËÍ
	 */
	virtual void on_tick(const tick_info& tick, const deal_info& deal)  override;



private:
	
	double_t _delta ;

	double_t _max_volume_key;

	std::map<double_t,uint32_t> _price_volume ;
};

