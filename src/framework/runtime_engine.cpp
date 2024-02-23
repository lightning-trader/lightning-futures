#include <runtime_engine.h>
#include <thread>
#include <time_utils.hpp>

using namespace lt;

runtime_engine::runtime_engine(const char* config_path):engine(CT_RUNTIME, config_path)
{
}
runtime_engine::~runtime_engine()
{
}


void runtime_engine::start_trading(const std::vector<std::shared_ptr<lt::strategy>>& strategys)
{
	if (!lt_login_account(_lt))
	{
		return;
	}
	regist_strategy(strategys);
	if(lt_start_service(_lt))
	{
		LOG_INFO("runtime_engine run in start_trading");
	}
}

void runtime_engine::stop_trading()
{
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	if(lt_stop_service(_lt))
	{
		clear_strategy();
		lt_logout_account(_lt);
		LOG_INFO("runtime_engine run end");
	}
}
