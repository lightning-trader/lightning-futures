#include "gallop.h"
#include <define.h>
#include "demo_strategy.h"
#include "hcc_strategy.h"
#include "emg_1_strategy.h"
#include "emg_2_strategy.h"
#include "runtime_engine.h"
#include "evaluate_engine.h"
#include "config.h"
#include "time_utils.hpp"

#pragma comment (lib,"lightning.lib")
#pragma comment (lib,"libltpp.lib")



std::shared_ptr<std::map<straid_t, std::shared_ptr<strategy>>> make_strategys(const std::vector<strategy_info>& stra_info)
{
	auto result = std::make_shared<std::map<straid_t, std::shared_ptr<strategy>>>();
	for(auto it : stra_info)
	{
		LOG_INFO("make_strategys : %d %d %s", it.id, it.type, it.param.c_str());
		const auto& p = strategy::param(it.param.c_str());
		switch(it.type)
		{
			case ST_EMG_1:
				(*result)[it.id] = std::make_shared<emg_1_strategy>(p);
			break;
			case ST_EMG_2:
				(*result)[it.id] = std::make_shared<emg_2_strategy>(p);
				break;
		}
	}
	return result;
}

void start_runtime(const char * account_config, const char* strategy_config)
{
	auto app = std::make_shared<runtime_engine>(account_config);
	auto now = convert_to_uint(get_now()) ;
	const auto& stra_info = get_strategy_info(strategy_config,now);
	auto strategys = make_strategys(stra_info);
	app->run_to_close(*strategys);
	
}


void start_evaluate(const char* account_config, const char* strategy_config, const char* trading_day_config)
{
	auto app = std::make_shared<evaluate_engine>(account_config);
	auto config = get_strategy_evaluate(strategy_config, trading_day_config);
	for(auto it : config)
	{
		auto strategys = make_strategys(it.stra_info);
		app->back_test(*strategys, it.trading_days);
	}
}


int main(int argc,char* argv[])
{
	//start_runtime("rt_simnow.ini", 10, 1);
	start_evaluate("evaluate.ini","strategy_20w.xml", "trading_days.xml");
	return 0;
	if(argc < 4)
	{
		LOG_ERROR("start atgc error");
		return -1;
	}
	if (std::strcmp("evaluate", argv[1]) == 0)
	{
		if (argc < 5)
		{
			LOG_ERROR("start atgc error");
			return -1;
		}
		const char* account_file = argv[2];
		const char* strategy_file = argv[3];
		const char* trading_day_file = argv[4];
		LOG_INFO("start %s evaluate for %s %s %s", account_file, strategy_file, trading_day_file);
		start_evaluate(account_file, strategy_file, trading_day_file);
	}
	else
	{
		const char* account_file = argv[2];
		const char* strategy_file = argv[3];
		LOG_INFO("start %s runtime for %s %s", account_file, strategy_file);
		start_runtime(account_file, strategy_file);
	}
	return 0;
}
