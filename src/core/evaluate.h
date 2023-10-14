#pragma once
#include "context.h"

class evaluate : public context
{

private:
	
	class dummy_market* _market_simulator;

	class dummy_trader* _trader_simulator;

	std::shared_ptr<class csv_recorder> _recorder;

public:
	evaluate();
	virtual ~evaluate();
public:

	bool init_from_file(const std::string& config_path);

	void playback_history();

	void simulate_crossday(uint32_t trading_day);

	virtual trader_api& get_trader() override;

	virtual market_api& get_market() override;

	virtual void on_update() override;

	virtual bool is_terminaled() override;


};