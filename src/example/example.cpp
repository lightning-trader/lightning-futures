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

#include "example.h"
#include <define.h>
#include <thread>
#include "runtime_engine.h"
#include "evaluate_engine.h"
#include "marketing_strategy.h"
#include <time_utils.hpp>
#include "orderflow_strategy.h"
#include "arbitrage_strategy.h"


void start_runtime(const char* account_config)
{
	auto app = std::make_shared<lt::runtime_engine>(account_config);
	std::vector<std::shared_ptr<lt::strategy>> strategys;
	strategys.emplace_back(std::make_shared<marketing_strategy>(1, app.get(), "SHFE.rb2210", 1, 1));
	strategys.emplace_back(std::make_shared<marketing_strategy>(2, app.get(), "SHFE.hc2210", 1, 1));
	strategys.emplace_back(std::make_shared<orderflow_strategy>(3, app.get(), "SHFE.rb2210", 1, 1, 3, 3, 10));
	strategys.emplace_back(std::make_shared<orderflow_strategy>(4, app.get(), "SHFE.hc2210", 1, 1, 3, 3, 10));
	app->set_trading_filter([app](const code_t& code, offset_type offset, direction_type direction, uint32_t count, double_t price, order_flag flag)->bool {
		auto now = app->get_last_time();
		auto last_order = app->last_order_time();
		if (now - last_order < 1)
		{
			return false;
		}
		return true;
		});
	app->start_trading(strategys);
	time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	time_t delta_seconds = make_datetime(app->get_trading_day(), "15:00:00") - now;
	if (delta_seconds > 0)
	{
		LOG_INFO("runtime_engine waiting for close :", delta_seconds);
		std::this_thread::sleep_for(std::chrono::seconds(delta_seconds));
	}
	app->stop_trading();
}


void start_evaluate(const char* account_config, const std::vector<uint32_t>& trading_days)
{
	auto app = std::make_shared<lt::evaluate_engine>(account_config);
	std::vector<std::shared_ptr<lt::strategy>> strategys;
	strategys.emplace_back(std::make_shared<marketing_strategy>(1, app.get(), "SHFE.rb2210", 1, 1));
	//strategys.emplace_back(std::make_shared<orderflow_strategy>(2, app, "SHFE.rb2210", 1, 1, 3, 3, 10));
	//strategys.emplace_back(std::make_shared<arbitrage_strategy>(3, app, "SHFE.rb2210", "SHFE.rb2211", 10, 1));
	for (auto& trading_day : trading_days)
	{
		app->back_test(strategys, trading_day);
	}
}


int main(int argc, char* argv[])
{
	init_log("./log", 128);
	if (argc > 1)
	{
		start_runtime("runtime.ini");
	}
	else 
	{
		std::vector<uint32_t> trading_days{ 20220801, 20220802, 20220803, 20220804, 20220805 };
		start_evaluate("evaluate.ini", trading_days);
	}
	return 0;
}
