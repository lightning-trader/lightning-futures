#pragma once
#include <tick_loader.h>

class csv_tick_loader: public tick_loader
{
public:
	bool init(const std::string& root_path);

public:
	virtual void load_tick(std::vector<tick_info>& result, const code_t& code, uint32_t trade_day) override;

private:

	std::string _root_path ;
};