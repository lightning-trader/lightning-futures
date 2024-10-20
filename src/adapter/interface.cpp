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
#include <interface.h>
#include "market/ctp_api_market.h"
#include "market/tap_api_market.h"

#include "trader/ctp_api_trader.h"
#include "trader/tap_api_trader.h"

using namespace lt;
using namespace lt::driver;

std::unordered_map<std::string, std::string> _id_excg_map;

lt::actual_market* create_actual_market(const lt::params& config)
{
	auto market_type = config.get<std::string>("market");
	if (market_type == "ctp_api")
	{
		return new ctp_api_market(_id_excg_map, config);
	}
	if (market_type == "tap_api")
	{
		return new tap_api_market(_id_excg_map, config);
	}
	return nullptr;
}

void destory_actual_market(actual_market*& api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
actual_trader* create_actual_trader(const params& config)
{
	auto trader_type = config.get<std::string>("trader");
	if (trader_type == "ctp_api")
	{
		return new ctp_api_trader(_id_excg_map, config);
	}
	if (trader_type == "tap_api")
	{
		return new tap_api_trader(_id_excg_map, config);
	}
	return nullptr;
}

void destory_actual_trader(actual_trader*& api)
{
	if (nullptr != api)
	{
		delete api;
		api = nullptr;
	}
}
