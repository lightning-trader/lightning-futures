#include <runtime_engine.h>
#include "strategy_manager.h"

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
	getchar();
}