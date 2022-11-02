#pragma once
#include "tick_simulator.h"

extern "C"
{
	EXPORT_FLAG simulator* create_simulator(const boost::property_tree::ptree& config)
	{
		tick_simulator* smlt = new tick_simulator();
		if(smlt->init(config))
		{
			return smlt;
		}
		delete smlt ;
		return nullptr ;
	}

	EXPORT_FLAG void destory_simulator(simulator* smlt)
	{
		if (nullptr != smlt)
		{
			delete smlt;
			smlt = nullptr;
		}
	}
};