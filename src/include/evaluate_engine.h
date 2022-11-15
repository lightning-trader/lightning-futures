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

	void start(strategy& stra, const std::vector<uint32_t>& trading_days);

private:

	ltobj _lt;

};

