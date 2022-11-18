#pragma once
#include <define.h>
#include <lightning.h>
#include <strategy.h>

class runtime_engine
{

public:

	runtime_engine(const char* config_path);
	virtual ~runtime_engine();

public:

	void add_strategy(straid_t id, std::shared_ptr<strategy> stra);

	void run();

private:

	ltobj _lt;

	std::unique_ptr<class strategy_manager> _strategy_manager ;
};


