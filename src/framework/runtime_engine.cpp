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


void runtime_engine::run_to_close(const std::vector<std::shared_ptr<lt::strategy>>& strategys)
{
	if(!lt_login_account(_lt))
	{
		return ;
	}
	regist_strategy(strategys);
	lt_start_service(_lt);
	while (!is_trading_ready())
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	LOG_INFO("runtime_engine run in trading ready");
	time_t close_time = make_datetime( get_trading_day(), get_close_time());
	time_t now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	time_t delta_seconds = close_time - now_time;
	if(delta_seconds>0)
	{
		LOG_INFO("runtime_engine waiting for close :", delta_seconds);
		std::this_thread::sleep_for(std::chrono::seconds(delta_seconds));
	}
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	lt_stop_service(_lt);
	clear_strategy();
	lt_logout_account(_lt);
	LOG_INFO("runtime_engine run end");
}