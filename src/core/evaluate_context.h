#pragma once
#include "context.h"

class evaluate_context : public context
{

private:
	
	class simulator * _simulator;

public:
	evaluate_context();
	virtual ~evaluate_context();
public:

	bool init_from_file(const std::string& config_path);

	double get_money();

	void play(uint32_t tradeing_day);
	
protected:
	
	virtual void on_update() override;

	
};