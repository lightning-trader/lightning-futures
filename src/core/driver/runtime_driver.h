#pragma once
#include <define.h>
#include "../driver.h"
#include "../dll_mgr/recorder_dll_mgr.h"
#include "../dll_mgr/market_api_dll_mgr.h"
#include "../dll_mgr/trader_api_dll_mgr.h"

class runtime_driver : public driver
{


private:
	
	market_api_dll_mgr _market_dll ;
	trader_api_dll_mgr _trader_dll ;
	recorder_dll_mgr _recorder_dll;

	class actual_market_api* _market_api;
	class actual_trader_api* _trader_api;
	class recorder * _recorder;
public:

	runtime_driver();
 	virtual ~runtime_driver();
	
public:

	bool init_from_file(const std::string& config_path);

public:

	virtual void update() override;

	virtual void add_handle(std::function<void(event_type, const std::vector<std::any>&)> handle) override;

	virtual class market_api* get_market_api() override
	{
		return _market_api;
	}

	virtual class trader_api* get_trader_api() override
	{
		return _trader_api;
	}

	virtual class recorder* get_recorder() override
	{
		return _recorder;
	}

};