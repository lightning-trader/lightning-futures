#include <runtime_engine.h>
#include "strategy_manager.h"
#include <thread>

runtime_engine::runtime_engine(const char* config_path)
{
	_lt = lt_create_context(CT_RUNTIME, config_path);
	_strategy_manager = std::make_unique<strategy_manager>(_lt);
}
runtime_engine::~runtime_engine()
{
	lt_destory_context(_lt);
}

void runtime_engine::add_strategy(straid_t id,std::shared_ptr<strategy> stra)
{
	if(_strategy_manager&&stra)
	{
		_strategy_manager->regist_strategy(id,stra);
	}
}

void runtime_engine::run()
{
	lt_start_service(_lt);
	bool is_in_trading = lt_is_in_trading(_lt);
	while (!is_in_trading)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		is_in_trading = lt_is_in_trading(_lt);
	}
	LOG_INFO("runtime_engine run in trading true");
	while (is_in_trading)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		is_in_trading = lt_is_in_trading(_lt);
	}
	lt_stop_service(_lt);
}