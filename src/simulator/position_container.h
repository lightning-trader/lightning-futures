// simulator.h: 目标的头文件。

#pragma once
#include <data_types.hpp>
#include <spin_mutex.hpp>
#include <map>


struct position_item
{
	//仓位
	uint32_t	postion;
	//价格
	double_t	price;
	//冻结
	uint32_t	frozen;

	position_item() :
		postion(0),
		price(.0F),
		frozen(0)
	{}

	uint32_t usable()const
	{
		return postion - frozen;
	}

	bool empty()const
	{
		return postion == 0;
	}

	void clear()
	{
		postion = 0;
		price = .0F;
		frozen = 0;
	}
};

struct position_detail
{
	code_t id; //合约ID
	position_detail(const code_t& code) :id(code) {}
	//今仓
	position_item today_long;
	position_item today_short;

	//昨仓
	position_item yestoday_long;
	position_item yestoday_short;

	bool empty()const
	{
		return today_long.empty() && today_short.empty() && yestoday_long.empty() && yestoday_short.empty();
	}

	uint32_t get_total()const
	{
		return today_long.postion + today_short.postion + yestoday_long.postion + yestoday_short.postion;
	}

	int32_t get_real()const
	{
		return today_long.postion + yestoday_long.postion - (today_short.postion + yestoday_short.postion);
	}

	uint32_t get_long_position()const
	{
		return today_long.postion + yestoday_long.postion;
	}

	uint32_t get_short_position()const
	{
		return today_short.postion + yestoday_short.postion;
	}
	uint32_t get_long_frozen()const
	{
		return today_long.frozen + yestoday_long.frozen;
	}

	uint32_t get_short_frozen()const
	{
		return today_short.frozen + yestoday_short.frozen;
	}
	position_detail()
	{}
};

class position_container
{

private:
	
	mutable spin_mutex _mutex;
	std::map<code_t, position_detail> _position_info;
	
public:

	position_container();

	~position_container();

	void increase_position(const code_t& code, direction_type direction,double_t price,uint32_t volume);

	void reduce_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today,bool is_reduce_frozen=true);
	//冻结
	void frozen_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today);
	//解冻
	void thawing_position(const code_t& code, direction_type direction, uint32_t volume, bool is_today);

	position_detail get_position_info(const code_t& code)const;

	void get_all_position(std::vector<position_detail>& position)const;

	void clear();

};
