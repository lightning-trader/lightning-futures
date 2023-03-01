#include "gallop.h"
#include <define.h>
#include "demo_strategy.h"
#include "hcc_strategy.h"
#include "hft_1_strategy.h"
#include "hft_2_strategy.h"
#include "hft_2a_strategy.h"
#include "hft_3_strategy.h"
#include "runtime_engine.h"
#include "evaluate_engine.h"

#pragma comment (lib,"lightning.lib")
#pragma comment (lib,"libltpp.lib")

void start_runtime(const char * config_file,int account_type)
{
	auto app = std::make_shared<runtime_engine>(config_file);
	switch(account_type)
	{
		case 0:
			app->add_strategy(0, std::make_shared<hft_2_strategy>("SHFE.rb2305", 1, 0.0028F, 120, 5, 3, 2));
			break;
		case 10:
			app->add_strategy(0, std::make_shared<hft_2_strategy>("SHFE.rb2305", 2, 0.0028F, 120, 6, 5, 1));
			app->add_strategy(1, std::make_shared<hft_3_strategy>("SHFE.rb2305", 1, 9, 0.88F, 0.58F, 2));
			break;
		case 30:
			app->add_strategy(0, std::make_shared<hft_2_strategy>("SHFE.rb2305", 2, 0.0028F, 120, 6, 5, 1));
			app->add_strategy(1, std::make_shared<hft_3_strategy>("SHFE.rb2305", 1, 9, 0.88F, 0.58F, 2));
			app->add_strategy(2, std::make_shared<hft_2_strategy>("SHFE.ag2306", 1, 0.0038F, 120, 8, 7, 1));
			app->add_strategy(3, std::make_shared<hft_3_strategy>("SHFE.ag2306", 1, 12, 0.98F, 0.88F, 2));
			break;
		
		case 500:
			app->add_strategy(0, std::make_shared<hft_2_strategy>("SHFE.rb2305", 20, 0.0028F, 120, 6, 5, 1));
			app->add_strategy(1, std::make_shared<hft_3_strategy>("SHFE.rb2305", 10, 9, 0.88F, 0.58F, 2));
			app->add_strategy(2, std::make_shared<hft_2_strategy>("SHFE.rb2306", 20, 0.0028F, 120, 5, 3, 1));
			app->add_strategy(3, std::make_shared<hft_3_strategy>("SHFE.rb2306", 10, 8, 0.98F, 0.68F, 2));
			app->add_strategy(4, std::make_shared<hft_2_strategy>("SHFE.ag2306", 10, 0.0038F, 120, 8, 7, 1));
			app->add_strategy(5, std::make_shared<hft_3_strategy>("SHFE.ag2306", 10, 12, 0.98F, 0.88F, 2));
			app->add_strategy(6, std::make_shared<hft_2_strategy>("SHFE.ag2305", 10, 0.0038F, 120, 6, 5, 1));
			app->add_strategy(7, std::make_shared<hft_3_strategy>("SHFE.ag2305", 10, 10, 0.98F, 0.88F, 2));
			break;
	}


	app->run_to_close();
	
}


void start_evaluate(const std::vector<uint32_t>& all_trading_day)
{
	auto app = std::make_shared<evaluate_engine>("./evaluate.ini");
	//20w
	std::vector<std::shared_ptr<strategy>> stra_list;
	stra_list.emplace_back(new hft_2_strategy("SHFE.rb2301", 2, 0.0028F, 120, 6, 5, 1));
	stra_list.emplace_back(new hft_3_strategy("SHFE.rb2301", 1, 9, 0.88F, 0.58F, 2));
	
	//stra_list.emplace_back(new hft_2_strategy("SHFE.rb2305", 2, 0.0028F, 120, 5, 3, 1));
	//stra_list.emplace_back(new hft_3_strategy("SHFE.rb2305", 1, 8, 0.98F, 0.68F, 2));

	stra_list.emplace_back(new hft_2_strategy("SHFE.ag2212", 1, 0.0038F, 120, 8, 7, 1));
	stra_list.emplace_back(new hft_3_strategy("SHFE.ag2212", 1, 12, 0.98F, 0.88F, 2));

	//stra_list.emplace_back(new hft_2_strategy("SHFE.ag2301", 1, 0.0038F, 120, 6, 5, 1));
	//stra_list.emplace_back(new hft_3_strategy("SHFE.ag2301", 1, 10, 0.98F, 0.88F, 2));

	app->back_test(stra_list, all_trading_day);
}


int main(int argc,char* argv[])
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
		20220929,
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

	
	start_evaluate( trading_day_2301);
	//start_evaluate(trading_day_2305);
	return 0;
	const char* config_file = "rt_hx_zjh.ini";
	int account_type = 0;
	//获取参数
	if(argc >= 3)
	{
		config_file = argv[1];
		account_type = std::atoi(argv[2]);
	}
	LOG_INFO("start runtime %s for %d", config_file, account_type);
	start_runtime(config_file, account_type);
	return 0;
}
