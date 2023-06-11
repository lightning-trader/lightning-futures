// example.cpp: 目标的源文件。
//

#include "example.h"
#include <define.h>
#include "runtime_engine.h"
#include "evaluate_engine.h"
#include "marketing_strategy.h"


void start_runtime(const char* account_config,const code_t& code)
{
	lt::runtime_engine app(account_config);
	std::vector<std::shared_ptr<lt::strategy>> strategys;
	strategys.emplace_back(std::make_shared<marketing_strategy>(1, app, code,1, 1));
	app.run_to_close(strategys);
}


void start_evaluate(const char* account_config,const code_t& code , uint32_t trading_day)
{
	lt::evaluate_engine app(account_config);
	std::vector<std::shared_ptr<lt::strategy>> strategys;
	strategys.emplace_back(std::make_shared<marketing_strategy>(1, app, code, 1, 1));
	app.back_test(strategys, trading_day);
}


int main()
{
	start_evaluate("evaluate.ini","SHFE.rb2210", 20220801);
	//start_runtime("runtime.ini", "SHFE.rb2310");
	return 0;
}
