#pragma once
#include <define.h>
#include <lightning.h>

class runtime_engine
{

public:

	runtime_engine(const char* config_path);
	virtual ~runtime_engine();

public:
	
	void start(const strategy& stra);

private:

	ltobj _lt;

};


