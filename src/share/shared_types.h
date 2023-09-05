#pragma once
#include <define.h>
#include <data_types.hpp>
#include "event_center.hpp"


struct position_seed
{
	code_t id;
	//今仓
	uint32_t today_long;
	uint32_t today_short;

	//昨仓
	uint32_t history_long;
	uint32_t history_short;

	position_seed():today_long(0U), today_short(0U), history_long(0U), history_short(0){}
};

struct trader_data
{
	
	std::vector<order_info> orders;

	std::vector<position_seed> positions;

};


typedef std::map<code_t, position_info> position_map;
//
typedef std::map<estid_t, order_info> entrust_map;

struct account_info
{
	double money;

	double frozen_monery;

	account_info() :
		money(.0F),
		frozen_monery(.0F)

	{}
};
