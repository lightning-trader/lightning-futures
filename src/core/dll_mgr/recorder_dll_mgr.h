#pragma once
#include "dll_mgr.h"
#include <recorder.h>
#include <platform_helper.hpp>
#include <boost/property_tree/ptree.hpp>

class recorder_dll_mgr : public dll_mgr
{

private:
	//ÐÐÇéapi
	typedef recorder* (*create_recorder_function)(const boost::property_tree::ptree&);
	typedef void(*destory_recorder_function)(recorder*);

private:
	
	DllHandle _recorder_handle;

public:
	
	
	virtual bool load(const std::string& file_name) override;

	virtual void unload() override;

public:

	create_recorder_function create_recorder;

	destory_recorder_function destory_recorder;

};