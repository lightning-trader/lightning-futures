#pragma once
#include "dll_mgr.h"
#include <market_api.h>
#include <platform_helper.hpp>

class market_api_dll_mgr : public dll_mgr
{

private:
	//ÐÐÇéapi
	typedef actual_market_api* (*create_market_function)(const boost::property_tree::ptree&);
	typedef void(*destory_market_function)(actual_market_api*);

private:
	
	DllHandle _market_handle;

public:
	
	
	virtual bool load(const std::string& file_name) override;

	virtual void unload() override;

public:

	create_market_function create_market_api;

	destory_market_function destory_market_api;

};