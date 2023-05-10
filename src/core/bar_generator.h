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

	uint32_t _prev_volume;

	std::map<double_t,uint32_t> _poc_data;

	std::function<void(const bar_info& )> _bar_finish;
	
public:

	bar_generator(uint32_t period,std::function<void(const bar_info& )> bar_finish) :_period(period), _minute(0), _prev_volume(0), _bar_finish(bar_finish) {}

	void insert_tick(const tick_info& tick);
};
