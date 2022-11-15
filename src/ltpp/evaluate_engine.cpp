#include "evaluate_engine.h"
#include <log_wapper.hpp>


evaluate_engine::evaluate_engine(const char* config_path)
{
	_lt = lt_create_context(CT_EVALUATE, config_path);
}
evaluate_engine::~evaluate_engine()
{
	lt_destory_context(_lt);
}

void evaluate_engine::start(strategy& stra, const std::vector<uint32_t>& trading_days)
{
	stra.init(_lt);
	lt_start_service(_lt);
	for(auto it : trading_days)
	{
		lt_playback_history(_lt,it);
	}
}