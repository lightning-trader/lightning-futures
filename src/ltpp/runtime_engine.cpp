#include "runtime_engine.h"
#include <log_wapper.hpp>
#include "lightning.h"
#include "strategy.h"

runtime_engine::runtime_engine(const char* config_path)
{
	_lt = create_context(CT_RUNTIME, config_path);
}
runtime_engine::~runtime_engine()
{
	destory_context(_lt);
}

void runtime_engine::start(const strategy& stra)
{
	stra.init(_lt);
	start_service(_lt);
}