#include <runtime_engine.h>
#include <log_wapper.hpp>

runtime_engine::runtime_engine(const char* config_path)
{
	_lt = lt_create_context(CT_RUNTIME, config_path);
}
runtime_engine::~runtime_engine()
{
	lt_destory_context(_lt);
}

void runtime_engine::start(strategy& stra)
{
	stra.init(_lt);
	lt_start_service(_lt);
}