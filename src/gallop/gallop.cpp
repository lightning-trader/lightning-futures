#include "gallop.h"
#include <define.h>
#include "demo_strategy.h"
#include "hcc_strategy.h"
#include "hft_1_strategy.h"
#include "hft_2_strategy.h"
#include "hft_3_strategy.h"
#include "runtime_engine.h"
#include "evaluate_engine.h"

#pragma comment (lib,"lightning.lib")
#pragma comment (lib,"libltpp.lib")

void start_runtime() 
{
	//auto app = std::make_shared<runtime_engine>("./runtime.ini");
	auto app = std::make_shared<runtime_engine>("./rt_hx_zjh.ini");
	app->add_strategy(0, std::make_shared<hft_2_strategy>("SHFE.rb2306", 0.0058F, 120, 8, 3));
	app->add_strategy(1, std::make_shared<hft_2_strategy>("SHFE.rb2305", 0.0028F, 120, 5, 2));
	//app->add_strategy(2, std::make_shared<hft_3_strategy>("SHFE.rb2305", 12, 0.58F, 1.28F, 1.5F, 3));
	//app->add_strategy(3, std::make_shared<hft_3_strategy>("SHFE.rb2305", 9, 0.88F, 0.28F, 1.5F, 2));
	app->run_to_close();
	
}

void start_evaluate(const char* code,const std::vector<uint32_t>& all_trading_day)
{
	auto app = std::make_shared<evaluate_engine>("./evaluate.ini");
	/*
	std::vector<uint32_t> trading_day = { 
		20220801,
		20220802,
		20220803
		};
	*/
	std::vector<std::shared_ptr<strategy>> stra_list;
	//stra_list.emplace_back(new hft_2_strategy("SHFE.rb2305", 0.0014F, 120, 3, 1));
	stra_list.emplace_back(new hft_2_strategy("SHFE.rb2301", 0.0028F, 120, 5, 2));
	///stra_list.emplace_back(new hft_2_strategy("SHFE.rb2305", 0.0058F, 120, 8, 3));
	//stra_list.emplace_back(new hft_3_strategy("SHFE.rb2301", 8, 0.88F, 0.58F, 1.58F, 3));
	stra_list.emplace_back(new hft_3_strategy("SHFE.rb2305", 9, 0.58F, 1.28F, 1.58F, 2));

	app->back_test(stra_list, all_trading_day);
}

void start_hft3_evaluate(const char* code, const std::vector<uint32_t>& all_trading_day)
{
	auto app = std::make_shared<evaluate_engine>("./evaluate.ini");
	/*
	std::vector<uint32_t> trading_day = {
		20220801,
		20220802,
		20220803
		};
	*/
	std::vector<std::shared_ptr<strategy>> stra_list;
	//stra_list.emplace_back(new hft_2_strategy("SHFE.rb2305", 0.0014F, 120, 3, 1));
	stra_list.emplace_back(new hft_3_strategy("SHFE.rb2301", 8, 1.5F, 1.5F, 2.0F, 2));
	//stra_list.emplace_back(new hft_3_strategy("SHFE.rb2305", 0.0058F, 8, 3));
	//stra_list.emplace_back(new hft_2_strategy(code, 0.0098F, 120,16,5));
	//stra_list.emplace_back(new hft_2_strategy(code, 0.0126F,120,24,6));
	//stra_list.emplace_back(new hft_2_strategy("SHFE.rb2212", 16));
	app->back_test(stra_list, all_trading_day);
}
void start_hft1_evaluate(const std::vector<uint32_t>& all_trading_day)
{
	auto app = std::make_shared<evaluate_engine>("./evaluate.ini");
	/*
	std::vector<uint32_t> trading_day = {
		20220801,
		20220802,
		20220803
		};
	*/
	std::vector<std::shared_ptr<strategy>> stra_list;
	stra_list.emplace_back(new hft_1_strategy("SHFE.rb2301", 8, 16, 300, 0));
	stra_list.emplace_back(new hft_1_strategy("SHFE.rb2210", 8, 16, 300, 0));
	app->back_test(stra_list, all_trading_day);
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

	std::vector<uint32_t> trading_day_2210 = {
		//2210
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
	std::vector<uint32_t> trading_day_2301 = {
		//2301
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
		20220930,
		20221010,
		20221011,
		20221012,
		20221013,
		20221014,
		20221017,
		20221018,
		20221019,
		20221020,
		20221021,
		20221024,
		20221025,
		20221026,
		20221027,
		20221028,
		20221031,
		20221101,
		20221102,
		20221103,
		20221104,
		20221107,
		20221108,
		20221109,
		20221110,
		20221111,
		20221114,
		20221115,
		20221116,
		20221117,
		20221118,
		20221121,
		20221122,
		20221123,
		20221124,
		20221125,
		20221128,
		20221129,
		20221130
	};
	std::vector<uint32_t> trading_day_2305 = {
		//2305
		20221201,
		20221202,
		20221205,
		20221206,
		20221207,
		20221208,
		20221209,
		20221212,
		20221213,
		20221214,
		20221215,
		20221216,
		20221219,
		20221220,
		20221221,
		20221222,
		20221223,
		20221226,
		20221227,
		20221228,
		20221229,
		20221230
			
	};

	
//max money : 99915.800000 i:[0] j:[3] k:[4] x:[2] y:[0]
	//start_evaluate("SHFE.rb2301", trading_day_2301);
	//start_hft3_evaluate("SHFE.rb2301", trading_day_2301);
	//start_hft3_evaluate("SHFE.rb2305", trading_day_2305);
	//start_hft1_evaluate(all_trading_day);
	//start_demo_optimize(all_trading_day);
	start_runtime();

	
	return 0;
}
