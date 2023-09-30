// example.cpp: 目标的源文件。
//

#include "example.h"
#include <define.h>
#include "runtime_engine.h"
#include "evaluate_engine.h"
#include "marketing_strategy.h"
#include <time_utils.hpp>
#include "orderflow_strategy.h"
#include "arbitrage_strategy.h"


void start_runtime(const char* account_config)
{
	lt::runtime_engine app(account_config);
	std::vector<std::shared_ptr<lt::strategy>> strategys;
	strategys.emplace_back(std::make_shared<marketing_strategy>(1, app, "SHFE.rb2210", 1, 1));
	strategys.emplace_back(std::make_shared<marketing_strategy>(2, app, "SHFE.hc2210", 1, 1));
	strategys.emplace_back(std::make_shared<orderflow_strategy>(3, app, "SHFE.rb2210", 1, 1, 3, 3, 10));
	strategys.emplace_back(std::make_shared<orderflow_strategy>(4, app, "SHFE.hc2210", 1, 1, 3, 3, 10));
	app.set_trading_filter([&app](const code_t& code, offset_type offset, direction_type direction, uint32_t count, double_t price, order_flag flag)->bool {
		auto now = app.get_last_time();
		auto last_order = app.last_order_time();
		if (now - last_order < 1)
		{
			return false;
		}
		return true;
		});
	app.run_to_close(strategys);
}


void start_evaluate(const char* account_config, const std::vector<uint32_t>& trading_days)
{
	lt::evaluate_engine app(account_config);
	std::vector<std::shared_ptr<lt::strategy>> strategys;
	strategys.emplace_back(std::make_shared<marketing_strategy>(1, app, "SHFE.rb2210", 1, 1));
	//strategys.emplace_back(std::make_shared<orderflow_strategy>(2, app, "SHFE.rb2210", 1, 1, 3, 3, 10));
	//strategys.emplace_back(std::make_shared<arbitrage_strategy>(3, app, "SHFE.rb2210", "SHFE.rb2211", 10, 1));
	for (auto& trading_day : trading_days)
	{
		app.back_test(strategys, trading_day);
	}
}


int main(int argc, char* argv[])
{

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
