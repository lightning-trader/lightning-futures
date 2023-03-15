#pragma once
#include <define.h>
#include <lightning.h>
#include <strategy.h>
#include <engine.h>

class evaluate_engine : public engine
{

public:

	evaluate_engine(const char* config_path);
	virtual ~evaluate_engine();

public:

	void back_test(const std::map<straid_t, std::shared_ptr<strategy>>& stra_map, const std::vector<uint32_t>& trading_days);



};

