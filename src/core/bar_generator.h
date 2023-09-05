#pragma once
#include "define.h"
#include "data_types.hpp"

/***  
* 
* bar 生成器
*/
class bar_generator
{

private:
	
	uint32_t _period;

	bar_info _bar;

	uint32_t _minute;

	uint64_t _prev_volume;

	double_t _price_step ;

	std::map<double_t,uint32_t> _poc_data;

	std::function<void(const bar_info& )> _bar_finish;
	
public:

	bar_generator(uint32_t period,double_t price_step,std::function<void(const bar_info& )> bar_finish) :_period(period), _price_step(price_step), _minute(0), _prev_volume(0), _bar_finish(bar_finish) {}

	void insert_tick(const tick_info& tick);
};
