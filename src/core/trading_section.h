#pragma once
#include <rapidcsv.h>


class trading_section
{

public:
	trading_section(const std::string& config_path);
	virtual ~trading_section();

private:
	rapidcsv::Document _config_csv;
	std::map<time_t,time_t> _trading_section;
public:

	void init(uint32_t trading_day, time_t last_time);

	bool is_in_trading(time_t last_time);

	time_t get_open_time();

	time_t get_clase_time();

};

