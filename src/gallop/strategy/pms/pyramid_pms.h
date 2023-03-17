#pragma once
#include "pms.h"

class pyramid_pms : public pms
{

public:

	pyramid_pms(const position_info& pos,uint32_t once, double_t beta, trading_optimal opt) :
		pms(pos),
		_once(once),
		_beta(beta),
		_opt(opt)
	{
	};


	~pyramid_pms() {};


public:

	virtual uint32_t get_sell_volume(offset_type offset) override;


	virtual uint32_t get_buy_volume(offset_type offset) override;


private:

	uint32_t _once;

	double_t _beta;

	trading_optimal _opt;

};

