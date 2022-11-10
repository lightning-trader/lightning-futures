#pragma once
#include "../driver.h"

class EXPORT_FLAG evaluate_driver : public driver
{

private:
	
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

	virtual market_api* get_market_api() override;
	
	virtual trader_api* get_trader_api() override;
	
};