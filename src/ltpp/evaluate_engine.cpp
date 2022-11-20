#include "evaluate_engine.h"
#include "strategy_manager.h"


evaluate_engine::evaluate_engine(const char* config_path)
{
	_lt = lt_create_context(CT_EVALUATE, config_path);
	_strategy_manager = std::make_unique<strategy_manager>(_lt);
}
evaluate_engine::~evaluate_engine()
{
	lt_destory_context(_lt);
}

void evaluate_engine::back_test(std::shared_ptr<strategy> stra, const std::vector<uint32_t>& trading_days)
{
	if(_strategy_manager)
	{
		_strategy_manager->regist_strategy(0,stra);
		lt_start_service(_lt);
		for (auto it : trading_days)
		{
			lt_playback_history(_lt, it);
		}
		lt_stop_service(_lt);
	}
}