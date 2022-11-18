#include "gallop.h"
#include <define.h>
#include "demo_strategy.h"
#include "hcc_strategy.h"
#include <log_wapper.hpp>
#include "hft_1_strategy.h"
#include "dm_strategy.h"
#include "runtime_engine.h"
#include "evaluate_engine.h"

#pragma comment (lib,"lightning.lib")

void start_runtime()
{
	auto app = runtime_engine("./runtime.ini");
	auto hcc = std::make_shared<hft_1_strategy>(8, 16, 300, 0);
	app.add_strategy(0,hcc);
	app.run();
	
}

void start_evaluate(const std::vector<uint32_t>& all_trading_day)
{
	auto app = evaluate_engine("./evaluate.ini");
	auto hcc = std::make_shared<hft_1_strategy>(8, 16, 300, 0);
	app.back_test(hcc, all_trading_day);
}



void start_hft_1_optimize(const std::vector<uint32_t>& all_trading_day)
{
/*
	//max money : 99890.400000 i:[1] j:[6]
	double max_monery = 0;
	for (int i = 1; i <= 15; i++)
	{
		for (int j = 2; j <= 20; j++)
		{
			for(int k=50;k<=200;k+=50)
			{
				double_t current_monery = 0;
				auto app = evaluate_engine("./evaluate.ini");
				hft_1_strategy hcc(i, j, k, 0);
				app.start(hcc, all_trading_day);
				if (current_monery > max_monery)
				{
					max_monery = current_monery;
				}
				if (current_monery > 2100000)
				{
					LOG_OPTIMIZE("max money : %f i:[%d] j:[%d] k:%d\n", current_monery, i, j, k);
				}
			}
			
		}
	}
*/
}

int main()
{
	/*
	std::vector<uint32_t> all_trading_day = {
		20220901,
		20220902,
		20220905,
		20220906,
		20220907,
		20220908,
		20220909,
		20220913,
		20220914,
		20220915,
		20220916,
		20220919,
		20220920,
		20220921,
		20220922,
		20220923,
		20220926,
		20220927,
		20220928,
		20220928,
		20220930
	};
	*/
	std::vector<uint32_t> all_trading_day = {
		20220801,
		20220802,
		20220803,
		20220804,
		20220805,
		20220808,
		20220809,
		20220810,
		20220811,
		20220812,
		20220815,
		20220816,
		20220817,
		20220818,
		20220819,
		20220822,
		20220823,
		20220824,
		20220825,
		20220826

	};
	
//max money : 99915.800000 i:[0] j:[3] k:[4] x:[2] y:[0]
	start_evaluate(all_trading_day);
	//start_hft_1_optimize(all_trading_day);
	//start_demo_optimize(all_trading_day);
	//start_runtime();
	//getchar();
	return 0;
}
