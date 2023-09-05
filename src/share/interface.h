#pragma once
#include "market_api.h"
#include "trader_api.h"
#include <params.hpp>


EXPORT_FLAG actual_market* create_actual_market(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map, const params& config);

EXPORT_FLAG void destory_actual_market(actual_market*& api);

EXPORT_FLAG actual_trader* create_actual_trader(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map, const params& config);

EXPORT_FLAG void destory_actual_trader(actual_trader*& api);


dummy_trader* create_dummy_trader(const params& config);

void destory_dummy_trader(dummy_trader*& api);

dummy_market* create_dummy_market(const params& config);

void destory_dummy_market(dummy_market*& api);

