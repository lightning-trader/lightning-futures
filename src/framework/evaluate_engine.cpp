#include "evaluate_engine.h"
#include <thread>

using namespace lt;

evaluate_engine::evaluate_engine(const char* config_path):engine(CT_EVALUATE, config_path)
{
	
}
evaluate_engine::~evaluate_engine()
{
	
}

void evaluate_engine::back_test(const std::vector<std::shared_ptr<lt::strategy>>& strategys, uint32_t trading_day)
{
	lt_simulate_crossday(_lt,trading_day);
	lt_login_account(_lt);
	regist_strategy(strategys);
	lt_start_service(_lt);
	lt_playback_history(_lt);
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	lt_stop_service(_lt);
	clear_strategy();
	lt_logout_account(_lt);
}