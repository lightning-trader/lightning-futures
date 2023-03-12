#pragma once
#include "context.h"

class runtime : public context
{


private:
	
	futures_market* _market_api;
	
	futures_trader* _trader_api;

public:

	runtime();
 	virtual ~runtime();
	
public:

	bool init_from_file(const std::string& config_path);

	virtual trader_api& get_trader() override;

	virtual market_api& get_market() override;

protected:

	virtual void update() override;

	virtual void add_handle(std::function<void(event_type, const std::vector<std::any>&)> handle) override;
};