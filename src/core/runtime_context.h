#pragma once
#include <define.h>
#include "context.h"

class runtime_context : public context
{


private:
	
	class actual_market_api* _market_api;
	class actual_trader_api* _trader_api;

public:

	runtime_context();
 	virtual ~runtime_context();
	
public:

	bool init_from_file(const std::string& config_path);

public:

	virtual void on_update() override;

	
};