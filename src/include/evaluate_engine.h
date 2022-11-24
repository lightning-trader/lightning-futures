#pragma once
#include <define.h>
#include <lightning.h>
#include <strategy.h>

class evaluate_engine
{

public:

	evaluate_engine(const char* config_path);
	virtual ~evaluate_engine();

public:

	void back_test(std::vector<std::shared_ptr<strategy>> stra_list, const std::vector<uint32_t>& trading_days);

private:

	ltobj _lt;

	std::unique_ptr<class strategy_manager> _strategy_manager;

};

