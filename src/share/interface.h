#pragma once
#include "market_api.h"
#include "trader_api.h"
#include "simulator.h"

market_api* create_market_api(event_source* evt, const boost::property_tree::ptree& config);

void destory_market_api(market_api* api);

trader_api* create_trader_api(event_source* evt, const boost::property_tree::ptree& config);

void destory_trader_api(trader_api* api);

simulator* create_simulator(event_source* evt, const boost::property_tree::ptree& config);

void destory_simulator(simulator* smlt);
