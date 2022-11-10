#include "gallop.h"
#include <define.h>
#include "lightning.h"
#include "demo_strategy.h"
#include "hcc_strategy.h"
#include <log_wapper.hpp>
#include "hft_1_strategy.h"
#include "dm_strategy.h"

#pragma comment (lib,"lightning.lib")

void start_runtime()
{
	auto dirver = create_runtime_driver("./runtime.ini");
	if(dirver)
	{
	//max money : 2106547.600000 i:[5] j:[4] k:5
		//hft_1_strategy hcc(2, 3, 6, 480, 480);
		//demo_strategy hcc(2,1);
		hft_1_strategy hcc(8, 16, 300, 0);
		context* app = create_context(dirver, &hcc);
		if(app)
		{
			app->start();
			destory_context(app);
		}
		destory_runtime_driver(dirver);
	}
}

void start_evaluate(const std::vector<uint32_t>& all_trading_day)
{
	auto dirver = create_evaluate_driver("./evaluate.ini");
	if (dirver)
	{
		hft_1_strategy hcc(8, 16, 300, 0);
		auto app = create_context(dirver, &hcc);
		if(app)
		{
			for (auto& trading : all_trading_day)
			{
				dirver->play(trading);
				app->start();
				//break;
			}
			destory_context(app);
		}
		destory_evaluate_driver(dirver);
	}
}



void start_hft_1_optimize(const std::vector<uint32_t>& all_trading_day)
{
/*
	//max money : 99890.400000 i:[1] j:[6]
	double max_monery = 0;
	for (int i = 7; i <= 15; i++)
	{
		for (int j = 1; j <= 20; j++)
		{
			for(int k=30;k<=30;k++)
			{
				double_t current_monery = 0;
				evaluate_driver dirver;
				if (dirver.init_from_file("./evaluate.ini"))
				{
					hft_1_strategy hcc(i, j, k,0 );
					//demo_strategy hcc(2, 7);
					//hcc_strategy hcc("SHFE.rb2301", 0.4, 10, 50, 120);
					execute_context app(dirver, hcc);
					for (auto& trading : all_trading_day)
					{
						dirver.play(trading);
						app.start();
						current_monery += dirver.get_money();
						//break;
					}
				}
				if (current_monery > max_monery)
				{
					max_monery = current_monery;
					LOG_OPTIMIZE("max money : %f i:[%d] j:[%d] k:%d\n", current_monery, i, j,k);
				}
			}
			
		}
	}
*/
}

int main()
{

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
	/*
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
		20220826,
		20220829,
		20220830,
		20220831
	};
	*/
//max money : 99915.800000 i:[0] j:[3] k:[4] x:[2] y:[0]
	start_evaluate(all_trading_day);
	//start_hft_1_optimize(all_trading_day);
	//start_demo_optimize(all_trading_day);
	//start_runtime();
	//getchar();
	return 0;
}
