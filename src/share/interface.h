#pragma once
#include "market_api.h"
#include "trader_api.h"
#include "simulator.h"

actual_market_api* create_market_api(const boost::property_tree::ptree& config);

void destory_market_api(actual_market_api* api);

actual_trader_api* create_trader_api(const boost::property_tree::ptree& config);

void destory_trader_api(actual_trader_api* api);

simulator* create_simulator(const boost::property_tree::ptree& config);

void destory_simulator(simulator* smlt);
