#pragma once
#include <rapidcsv.h>
#include <define.h>

class price_step
{

public:
	price_step(const std::string& config_path);
	virtual ~price_step();

private:

	std::map<code_t,double_t> _price_step_data;

public:
	
	double_t get_price_step(const code_t& code)const;

	double_t get_proximate_price(const code_t& code,double_t row_price)const;
};

