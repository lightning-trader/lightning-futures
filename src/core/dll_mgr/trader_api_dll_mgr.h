#pragma once
#include "dll_mgr.h"
#include <trader_api.h>
#include <platform_helper.hpp>

class trader_api_dll_mgr : public dll_mgr
{

private:
	//ÐÐÇéapi
	typedef actual_trader_api* (*create_trader_function)(const boost::property_tree::ptree&);
	typedef void(*destory_trader_function)(actual_trader_api*);

private:
	
	DllHandle _trader_handle;

public:
	
	
	virtual bool load(const std::string& file_name) override;

	virtual void unload() override;

public:

	create_trader_function create_trader_api;

	destory_trader_function destory_trader_api;

};