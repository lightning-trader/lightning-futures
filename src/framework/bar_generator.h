#pragma once
#include "define.h"
#include "data_types.hpp"
#include <receiver.h>

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

	std::set<lt::bar_receiver*> _bar_callback;
	
public:

	bar_generator(uint32_t period,double_t price_step) :_period(period), _price_step(price_step), _minute(0), _prev_volume(0) {}

	void insert_tick(const tick_info& tick);

	void add_receiver(lt::bar_receiver* receiver);

	void remove_receiver(lt::bar_receiver* receiver);

	bool invalid()const ;
};
