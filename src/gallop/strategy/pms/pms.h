#pragma once
#include "define.h"

class pms
{
	
protected:

	const position_info& _pos ;

public:

	pms(const position_info& pos):_pos(pos) {}

	/*
	 *	获取卖出数量
	 */
	virtual uint32_t get_sell_volume(offset_type offset) = 0;

	/**
	*  获取买入数量
	*/
	virtual uint32_t get_buy_volume(offset_type offset) = 0;
};

