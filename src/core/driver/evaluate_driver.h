#pragma once
#include "../driver.h"
#include <simulator.h>
#include "../dll_mgr/simulator_dll_mgr.h"
#include "../dll_mgr/recorder_dll_mgr.h"

class evaluate_driver : public driver
{

private:
	

	simulator_dll_mgr _simulator_dll;
	recorder_dll_mgr _recorder_dll;

	simulator* _simulator;
	class recorder* _recorder;

public:
	evaluate_driver();
	virtual ~evaluate_driver();
public:

	bool init_from_file(const std::string& config_path);

	void set_trading_day(uint32_t tradeing_day);

	double get_money();

	void play();
	
public:
	
	virtual void update() override;

	virtual void add_handle(std::function<void(event_type, const std::vector<std::any>&)> handle) override;

	virtual class market_api* get_market_api() override
	{
		return _simulator;
	}

	virtual class trader_api* get_trader_api() override
	{
		return _simulator;
	}

	virtual class recorder* get_recorder() override
	{
		return _recorder;
	}


};