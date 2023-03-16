#include "evaluate_engine.h"
#include "strategy_manager.h"
#include <chrono>
#include <thread>


evaluate_engine::evaluate_engine(const char* config_path)
{
	_lt = lt_create_context(CT_EVALUATE, config_path);
	_strategy_manager = std::make_unique<strategy_manager>(_lt);
}
evaluate_engine::~evaluate_engine()
{
	lt_destory_context(_lt);
}

void evaluate_engine::back_test(const std::map<straid_t,std::shared_ptr<strategy>>& stra_map, const std::vector<uint32_t>& trading_days)
{
	if(_strategy_manager)
	{
		for(auto it : stra_map)
		{
			_strategy_manager->regist_strategy(it.first, it.second);
		}
		lt_start_service(_lt);
		for (auto it : trading_days)
		{
			lt_playback_history(_lt, it);
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
		lt_stop_service(_lt);
		for (auto it : stra_map)
		{
			_strategy_manager->unregist_strategy(it.first);
		}
	}
}