// simulator.h: 目标的头文件。

#pragma once
#include <define.h>
#include <spin_mutex.hpp>
#include <map>

class position_container
{

private:
	
	mutable spin_mutex _mutex;
	std::map<code_t, position_info> _today_position;
	std::map<code_t, position_info> _history_position;
	std::map<code_t, double_t> _avg_long_price;
	std::map<code_t, double_t> _avg_short_price;
public:

	position_container();

	~position_container();

	void increase_position(const code_t& code, direction_type direction,double_t price,uint32_t volume);

	void reduce_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today,bool is_reduce_frozen=true);
	//冻结
	void frozen_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today);
	//解冻
	void thawing_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today);

	void crossday_move_position(std::function<void(const code_t&)> move_callback);

	position_info get_merge_position(const code_t& code)const;

	position_info get_today_position(const code_t& code)const;

	position_info get_history_position(const code_t& code)const;

	double_t get_avg_price(const code_t& code,direction_type direction)const;

	void get_all_position(std::map<code_t,position_info>& all_position)const;

	void clear();

	uint32_t get_history_usable(const code_t& code, direction_type direction)const;
};
