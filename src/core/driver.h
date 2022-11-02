#pragma once
#include <any>
#include <functional>
#include <event_center.hpp>

class driver
{
public:
	driver(){}
	virtual ~driver(){}

public:

	virtual void update() = 0;

	virtual void add_handle(std::function<void(event_type, const std::vector<std::any>&)> handle) = 0;
	
	virtual class market_api* get_market_api() = 0;

	virtual class trader_api* get_trader_api() = 0;

	virtual class recorder* get_recorder() = 0;

};