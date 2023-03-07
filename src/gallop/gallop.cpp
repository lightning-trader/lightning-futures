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

#ifndef NDEBUG
const char* rb_frist = "SHFE.rb2301";
const char* ag_frist = "SHFE.ag2212";
const char* rb_second = "SHFE.rb2302";
const char* ag_second = "SHFE.ag2301";
#else
const char* rb_frist = "SHFE.rb2305";
const char* ag_frist = "SHFE.ag2306";
const char* rb_second = "SHFE.rb2306";
const char* ag_second = "SHFE.ag2305";
#endif	

std::shared_ptr<std::map<straid_t, std::shared_ptr<strategy>>> make_strategys(int account_type, int multiple)
{


	auto result = std::make_shared<std::map<straid_t,std::shared_ptr<strategy>>>();
	switch (account_type)
	{
	case 0:
		(*result)[0] = std::make_shared<hft_2_strategy>(rb_frist, multiple, 0.0028F, 120, 5, 3, 2);
		break;
	case 20:
		(*result)[0] = std::make_shared<hft_2_strategy>(rb_frist, multiple, 0.0028F, 120, 5, 3, 2);
		(*result)[1] = std::make_shared<hft_3_strategy>(rb_second, multiple, 9, 1.98F, 0.36F, 8, 2);
		break;
	case 100:
		(*result)[0] = std::make_shared<hft_2_strategy>(rb_frist, multiple * 2, 0.0028F, 120, 5, 3, 2);
		(*result)[1] = std::make_shared<hft_3_strategy>(rb_second, multiple * 2, 9, 1.98F, 0.36F, 8, 2);
		(*result)[2] = std::make_shared<hft_2_strategy>(ag_second, multiple, 0.0049F, 120, 16, 3, 3);
		(*result)[3] = std::make_shared<hft_3_strategy>(ag_frist, multiple, 21, 1.58F, .0F, 16, 2);
		break;
	}
	return result;
}

void start_runtime(const char * config_file,int account_type,int multiple)
{
	auto app = std::make_shared<runtime_engine>(config_file);
	auto strategys = make_strategys(account_type, multiple);
	for(auto it : *strategys)
	{
		app->add_strategy(it.first,it.second);
	}
	app->run_to_close();
	
}


void start_evaluate(const std::vector<uint32_t>& all_trading_day)
{
	auto app = std::make_shared<evaluate_engine>("./evaluate.ini");
	
	app->set_trading_filter([app](const code_t& code, offset_type offset, direction_type direction ,uint32_t count,double_t price, order_flag flag)->bool{
		if(offset == OT_OPEN)
		{
			return true ;
		}
		auto pos = app->get_position(code);
		if (direction == DT_LONG)
		{
			if (pos.yestoday_long.postion >= count)
			{
				return price > pos.yestoday_long.price;
			}
			return price > pos.today_long.price;
		}
		if (direction == DT_SHORT)
		{
			if (pos.yestoday_short.postion >= count)
			{
				return price < pos.yestoday_short.price;
			}
			return price < pos.today_short.price;
		}
		return false;
	});
	
	//20w
	std::vector<std::shared_ptr<strategy>> stra_list;
	auto strategys = make_strategys(100, 1);
	for (auto it : *strategys)
	{
		stra_list.emplace_back(it.second);
	}
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
	int multiple = 1;
	//获取参数
	if(argc >= 2)
	{
		config_file = argv[1];
	}
	if (argc >= 3)
	{
		account_type = std::atoi(argv[2]);
	}
	if (argc >= 4)
	{
		multiple = std::atoi(argv[3]);
	}

	LOG_INFO("start runtime %s for %d*%d", config_file, account_type, multiple);
	start_runtime(config_file, account_type, multiple);
	return 0;
}
