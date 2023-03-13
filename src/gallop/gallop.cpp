#include "gallop.h"
#include <define.h>
#include "demo_strategy.h"
#include "hcc_strategy.h"
#include "emg_1_strategy.h"
#include "emg_2_strategy.h"
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

std::shared_ptr<std::map<straid_t, std::shared_ptr<strategy>>> make_strategys(const char* rb_frist,const char* hc_frist, const char* ag_frist,int account_type, int multiple)
{

	LOG_INFO("make_strategys : %s %s %s", rb_frist, ag_frist, hc_frist);

	auto result = std::make_shared<std::map<straid_t,std::shared_ptr<strategy>>>();
	switch (account_type)
	{
	case 0:
		//(*result)[0] = std::make_shared<hcc_strategy>(rb_frist, multiple, 0.0036F, 3, 60);
		break;
	case 10:
		(*result)[0] = std::make_shared<emg_1_strategy>(rb_frist, multiple, 0.0036F, 3, 8, 2.F, 2);
		break;
	case 20:
		(*result)[0] = std::make_shared<emg_2_strategy>(rb_frist, multiple, 9, 0.39F, 0.8F, 18, 1);
		break;
	case 30:
		(*result)[0] = std::make_shared<emg_1_strategy>(hc_frist, multiple, 0.0036F, 3, 8, 2.F, 2);
		(*result)[1] = std::make_shared<emg_2_strategy>(rb_frist, multiple, 9, 0.39F, 0.8F, 18, 1);
		break;
	case 50:
		(*result)[0] = std::make_shared<emg_1_strategy>(hc_frist, multiple * 3, 0.0036F, 3, 8, 2.F, 2);
		(*result)[1] = std::make_shared<emg_2_strategy>(rb_frist, multiple, 9, 0.39F, 0.8F, 18, 1);
		break;
	case 60:
		(*result)[0] = std::make_shared<emg_2_strategy>(rb_frist, multiple * 2, 9, 0.39F, 0.8F, 18, 1);
		(*result)[1] = std::make_shared<emg_2_strategy>(ag_frist, multiple, 16, 0.58F, 0.98F, 12, 1);
		break;
	case 100:
		(*result)[0] = std::make_shared<emg_1_strategy>(hc_frist, multiple * 3 , 0.0036F, 3, 8, 2.F, 2);
		(*result)[1] = std::make_shared<emg_2_strategy>(rb_frist, multiple * 2, 9, 0.39F, 0.8F, 18, 1);
		(*result)[2] = std::make_shared<emg_2_strategy>(ag_frist, multiple, 16, 0.58F, 0.98F, 12, 1);
		break;
	}
	return result;
}

void start_runtime(const char * config_file,int account_type,int multiple)
{
	auto app = std::make_shared<runtime_engine>(config_file);
	app->bind_transfer_info("SHFE.rb2310", "SHFE.rb2305", 104);
	app->bind_transfer_info("SHFE.hc2310", "SHFE.rb2306", -7);
	auto strategys = make_strategys("SHFE.rb2310","SHFE.hc2310", "SHFE.ag2306",account_type, multiple);
	for(auto it : *strategys)
	{
		app->add_strategy(it.first,it.second);
	}
	app->run_to_close();
	
}


void start_evaluate(const char* config_file, int account_type, int multiple)
{
	auto app = std::make_shared<evaluate_engine>(config_file);

	for(auto it : trading_index)
	{
		uint32_t index = it.first;
		auto rb_frist = get_rb_frist(index);
		auto rb_second = get_rb_second(index);
		if(std::strcmp(rb_frist.first,"")!=0 && rb_frist.second != default_transfer)
		{
			app->bind_transfer_info(rb_frist.first, rb_frist.second.expire_code, rb_frist.second.price_offset);
		}
		if (std::strcmp(rb_second.first, "") != 0 && rb_frist.second != default_transfer)
		{
			app->bind_transfer_info(rb_second.first, rb_second.second.expire_code, rb_second.second.price_offset);
		}
		std::vector<std::shared_ptr<strategy>> stra_list;
		auto strategys = make_strategys(rb_frist.first,rb_second.first,"SHFE.ag2306", account_type, multiple);
		for (auto it : *strategys)
		{
			stra_list.emplace_back(it.second);
		}
		app->back_test(stra_list, it.second);
	}

	
}


int main(int argc,char* argv[])
{
	//start_runtime("rt_hx_zjh.ini", 10, 1);
	//start_evaluate("evaluate.ini",0, 1);
	//return 0;
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
		start_evaluate(config_file, account_type, multiple);
	}
	else
	{
		LOG_INFO("start %s runtime for %d*%d", config_file, account_type, multiple);
		start_runtime(config_file, account_type, multiple);
	}
	return 0;
}
