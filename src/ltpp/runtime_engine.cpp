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


void runtime_engine::run(const std::string& end_time)
{
	lt_start_service(_lt);
	bool is_trading_ready = lt_is_trading_ready(_lt);
	while (!is_trading_ready)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		is_trading_ready = lt_is_trading_ready(_lt);
	}
	LOG_INFO("runtime_engine run in trading ready");
	uint32_t trading_day = lt_get_trading_day(_lt);
	time_t close_time = make_datetime(trading_day, end_time.c_str());
	time_t now_time = get_now();
	std::this_thread::sleep_for(std::chrono::seconds(close_time - now_time));
	lt_stop_service(_lt);
}