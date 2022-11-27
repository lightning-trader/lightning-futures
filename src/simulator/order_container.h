// simulator.h: 目标的头文件。

#pragma once
#include <define.h>
#include <data_types.hpp>
#include <spin_mutex.hpp>
#include <map>

enum order_state
{
	OS_INVALID,
	OS_IN_MATCH,
	OS_CANELED,
};

struct order_match
{
	estid_t		est_id;
	uint32_t	queue_seat; //队列前面有多少个
	order_state		state;
	order_flag		flag;


	order_match(estid_t id, order_flag flg):est_id(id), queue_seat(0), state(OS_INVALID), flag(flg)
	{}
};

class order_container
{

private:
	
	mutable spin_mutex _mutex ;

	std::map<estid_t, order_info> _order_info;

	std::map<code_t, std::vector<order_match>> _order_match;

	
public:

	void add_order(const order_info& order_info, order_flag flag);

	void del_order(estid_t estid);

	void set_seat(estid_t estid,uint32_t seat);

	void set_state(estid_t estid, order_state state);

	uint32_t set_last_volume(estid_t estid, uint32_t last_volume);

	double_t set_price(estid_t estid, double_t price);

	void get_order_match(std::vector<order_match>& match_list, const code_t& code)const;

	bool get_order_info(order_info& order, estid_t estid)const;
	
	const std::map<estid_t, order_info> get_all_order()const;

	void clear();
};
