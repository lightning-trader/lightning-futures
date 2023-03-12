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
#include "trading_day.h"

#pragma comment (lib,"lightning.lib")
#pragma comment (lib,"libltpp.lib")

typedef enum run_type
{
	RT_EVALUATE,
	RT_RUNTIME,
}run_type;

std::shared_ptr<std::map<straid_t, std::shared_ptr<strategy>>> make_strategys(run_type rt,int account_type, int multiple)
{
	const char* rb_frist = "SHFE.rb2305";
	const char* ag_frist = "SHFE.ag2306";
	const char* rb_second = "SHFE.rb2306";
	const char* ag_second = "SHFE.ag2305";
	if(rt == RT_EVALUATE)
	{
		rb_frist = "SHFE.rb2301";
		ag_frist = "SHFE.ag2212";
		rb_second = "SHFE.rb2302";
		ag_second = "SHFE.ag2301";
	}
	LOG_INFO("make_strategys : %s %s %s %s", rb_frist, ag_frist, rb_second, ag_second);

	auto result = std::make_shared<std::map<straid_t,std::shared_ptr<strategy>>>();
	switch (account_type)
	{
	case 10:
		(*result)[0] = std::make_shared<hft_2_strategy>(rb_frist, multiple, 0.0036F, 3, 8, 2.F, 2);
		break;
	case 20:
		(*result)[0] = std::make_shared<hft_3_strategy>(rb_frist, multiple, 9, 0.39F, 0.8F, 18, 1);
		break;
	case 30:
		(*result)[0] = std::make_shared<hft_2_strategy>(rb_second, multiple, 0.0036F, 3, 8, 2.F, 2);
		(*result)[1] = std::make_shared<hft_3_strategy>(rb_frist, multiple, 9, 0.39F, 0.8F, 18, 1);
		break;
	case 50:
		(*result)[0] = std::make_shared<hft_2_strategy>(rb_second, multiple * 3, 0.0036F, 3, 8, 2.F, 2);
		(*result)[1] = std::make_shared<hft_3_strategy>(rb_frist, multiple, 9, 0.39F, 0.8F, 18, 1);
		break;
	case 60:
		(*result)[0] = std::make_shared<hft_3_strategy>(rb_frist, multiple * 2, 9, 0.39F, 0.8F, 18, 1);
		(*result)[1] = std::make_shared<hft_3_strategy>(ag_frist, multiple, 16, 0.58F, 0.98F, 12, 1);
		break;
	case 100:
		(*result)[0] = std::make_shared<hft_2_strategy>(rb_second, multiple * 3 , 0.0036F, 3, 8, 2.F, 2);
		(*result)[1] = std::make_shared<hft_3_strategy>(rb_frist, multiple * 2, 9, 0.39F, 0.8F, 18, 1);
		(*result)[2] = std::make_shared<hft_3_strategy>(ag_frist, multiple, 16, 0.58F, 0.98F, 12, 1);
		break;
	}
	return result;
}

void start_runtime(run_type rt, const char * config_file,int account_type,int multiple)
{
	auto app = std::make_shared<runtime_engine>(config_file);
	
	auto strategys = make_strategys(rt,account_type, multiple);
	for(auto it : *strategys)
	{
		app->add_strategy(it.first,it.second);
	}
	app->run_to_close();
	
}


void start_evaluate(const std::vector<uint32_t>& all_trading_day, const char* config_file, int account_type, int multiple)
{
	auto app = std::make_shared<evaluate_engine>(config_file);
	std::vector<std::shared_ptr<strategy>> stra_list;
	auto strategys = make_strategys(RT_EVALUATE,account_type, multiple);
	for (auto it : *strategys)
	{
		stra_list.emplace_back(it.second);
	}
	app->back_test(stra_list, all_trading_day);
}


int main(int argc,char* argv[])
{
	//start_runtime("rt_hx_zjh.ini", 10, 1);
	start_evaluate(trading_day_2301, "evaluate.ini",10, 1);
	return 0;
	if(argc < 3)
	{
		LOG_ERROR("start atgc error");
		return -1;
	}
	const char* config_file = argv[2];

	int account_type = 10;
	int multiple = 1;
	//获取参数
	
	if (argc >= 4)
	{
		account_type = std::atoi(argv[3]);
	}
	if (argc >= 5)
	{
		multiple = std::atoi(argv[4]);
	}
	
	if (std::strcmp("evaluate", argv[1])==0)
	{
		LOG_INFO("start %s evaluate for %d*%d", config_file, account_type, multiple);
		start_evaluate(trading_day_2301, config_file, account_type, multiple);
	}
	else
	{
		LOG_INFO("start %s runtime for %d*%d", config_file, account_type, multiple);
		start_runtime(RT_RUNTIME,config_file, account_type, multiple);
	}
	return 0;
}
