// simulator.h: 目标的头文件。

#pragma once
#include <define.h>
#include <spin_mutex.hpp>
#include <map>

class position_container
{

private:
	
	mutable spin_mutex _mutex;
	std::map<code_t, position_info> _position_info;
	
public:

	position_container();

	~position_container();

	void increase_position(const code_t& code, direction_type direction,double_t price,uint32_t volume);

	void reduce_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today,bool is_reduce_frozen=true);
	//冻结
	void frozen_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today);
	//解冻
	void thawing_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today);

	position_info get_position_info(const code_t& code)const;

	void get_all_position(std::vector<position_info>& position)const;

	void clear();

};
