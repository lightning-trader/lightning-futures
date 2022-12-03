#include <runtime_engine.h>
#include "strategy_manager.h"
#include <thread>
#include <time_utils.hpp>

runtime_engine::runtime_engine(const char* config_path)
{
	_lt = lt_create_context(CT_RUNTIME, config_path);
	_strategy_manager = std::make_unique<strategy_manager>(_lt);
}
runtime_engine::~runtime_engine()
{
	lt_destory_context(_lt);
}


void runtime_engine::run_to_close()
{
	lt_start_service(_lt);
	bool is_trading_ready = lt_is_trading_ready(_lt);
	while (!is_trading_ready)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		is_trading_ready = lt_is_trading_ready(_lt);
	}
	LOG_INFO("runtime_engine run in trading ready");
	time_t close_time = lt_get_close_time(_lt);
	time_t now_time = get_now();
	time_t delta_seconds = close_time - now_time;
	if(delta_seconds>0)
	{
		std::this_thread::sleep_for(std::chrono::seconds(delta_seconds));
	}
	lt_stop_service(_lt);
}