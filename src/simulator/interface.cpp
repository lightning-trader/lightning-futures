#pragma once
#include "tick_simulator.h"

simulator* create_simulator(event_source* evt,const boost::property_tree::ptree& config)
{
	tick_simulator* smlt = new tick_simulator(evt);
	if (smlt->init(config))
	{
		return smlt;
	}
	delete smlt;
	return nullptr;
}

void destory_simulator(simulator* smlt)
{
	if (nullptr != smlt)
	{
		delete smlt;
		smlt = nullptr;
	}
}