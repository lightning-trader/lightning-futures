#pragma once
#include <define.h>
#include <lightning.h>
#include <engine.h>

class runtime_engine : public engine
{

public:

	runtime_engine(const char* config_path);
	virtual ~runtime_engine();

public:
	
	void run_to_close(const std::map<straid_t, std::shared_ptr<strategy>>& stra_map);

};


