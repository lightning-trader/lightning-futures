#pragma once
#include <define.h>
#include "context.h"

class runtime : public context
{


private:
	
	market_api* _market_api;
	
	trader_api* _trader_api;

public:

	runtime();
 	virtual ~runtime();
	
public:

	bool init_from_file(const std::string& config_path);

	
	virtual trader_api* get_trader() override;

	virtual market_api* get_market() override;
};