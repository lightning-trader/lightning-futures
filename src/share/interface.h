#pragma once
#include "market_api.h"
#include "trader_api.h"
#include "recorder.h"
#include <params.hpp>


actual_market* create_actual_market(const params& config);

void destory_actual_market(actual_market*& api);

actual_trader* create_actual_trader(const params& config);

void destory_actual_trader(actual_trader*& api);


dummy_trader* create_dummy_trader(const params& config);

void destory_dummy_trader(dummy_trader*& api);

dummy_market* create_dummy_market(const params& config);

void destory_dummy_market(dummy_market*& api);



recorder* create_recorder(const params& config);

void destory_recorder(recorder*& rcd);
