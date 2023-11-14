#pragma once
#include <define.h>

class trading_section
{

public:
	
	trading_section(const std::string& config_path);
	virtual ~trading_section();

private:
	std::vector<std::pair<daytm_t, daytm_t>> _trading_section;

public:
	
	bool is_in_trading(daytm_t last_time);

	daytm_t get_open_time();

	daytm_t get_close_time();

	daytm_t next_open_time(daytm_t now);

};

