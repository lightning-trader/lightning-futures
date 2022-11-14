#include "evaluate_engine.h"
#include <lightning.h>
#include <log_wapper.hpp>
#include "strategy.h"

evaluate_engine::evaluate_engine(const char* config_path)
{
	_lt = create_context(CT_EVALUATE, config_path);
}
evaluate_engine::~evaluate_engine()
{
	destory_context(_lt);
}

void evaluate_engine::start(const strategy& stra, const std::vector<uint32_t> trading_days)
{
	stra.init(_lt);
	start_service(_lt);
	for(auto it : trading_days)
	{
		playback_history(_lt,it);
	}
}