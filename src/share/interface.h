/*
Distributed under the MIT License(MIT)

Copyright(c) 2023 Jihua Zou EMail: ghuazo@qq.com QQ:137336521

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in the
Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once
#include "market_api.h"
#include "trader_api.h"
#include <params.hpp>


EXPORT_FLAG actual_market* create_actual_market(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map, const params& config);

EXPORT_FLAG void destory_actual_market(actual_market*& api);

EXPORT_FLAG actual_trader* create_actual_trader(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map, const params& config);

EXPORT_FLAG void destory_actual_trader(actual_trader*& api);


EXPORT_FLAG dummy_trader* create_dummy_trader(const params& config);

EXPORT_FLAG void destory_dummy_trader(dummy_trader*& api);

EXPORT_FLAG dummy_market* create_dummy_market(const params& config);

EXPORT_FLAG void destory_dummy_market(dummy_market*& api);

