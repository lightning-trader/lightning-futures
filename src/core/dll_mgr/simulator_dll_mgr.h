#pragma once
#include "dll_mgr.h"
#include <simulator.h>
#include <platform_helper.hpp>

class simulator_dll_mgr : public dll_mgr
{

private:
	//ÐÐÇéapi
	typedef simulator* (*create_simulator_function)(boost::property_tree::ptree&);
	typedef void(*destory_simulator_function)(simulator*);

private:
	
	DllHandle _simulator_handle;

public:
	
	
	virtual bool load(const std::string& file_name) override;

	virtual void unload() override;

public:

	create_simulator_function create_simulator;

	destory_simulator_function destory_simulator;

};