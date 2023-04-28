#pragma once
#include "define.h"

struct tick_loader
{
public:


	virtual void load_tick(std::vector<tick_info>& result, const code_t& code, uint32_t trade_day) = 0;
};