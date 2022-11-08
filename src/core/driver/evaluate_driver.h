#pragma once
#include "../driver.h"
#include "../dll_mgr/simulator_dll_mgr.h"

class evaluate_driver : public driver
{

private:
	

	simulator_dll_mgr _simulator_dll;
	
	class simulator* _simulator;

public:
	evaluate_driver();
	virtual ~evaluate_driver();
public:

	bool init_from_file(const std::string& config_path);

	double get_money();

	void play(uint32_t tradeing_day);
	
public:
	
	virtual void update() override;

	virtual void add_handle(std::function<void(event_type, const std::vector<std::any>&)> handle) override;

	virtual class market_api* get_market_api() override;
	
	virtual class trader_api* get_trader_api() override;
	
};