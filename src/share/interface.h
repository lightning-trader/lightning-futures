#pragma once
#include "market_api.h"
#include "trader_api.h"
#include "simulator.h"
#include "recorder.h"
#include <boost/property_tree/ptree.hpp>


futures_market* create_market_api(const boost::property_tree::ptree& config);

void destory_market_api(futures_market*& api);

futures_trader* create_trader_api(const boost::property_tree::ptree& config);

void destory_trader_api(futures_trader*& api);

simulator* create_simulator(const boost::property_tree::ptree& config);

void destory_simulator(simulator*& smlt);

recorder* create_recorder(const boost::property_tree::ptree& config);

void destory_recorder(recorder*& rcd);
