// psyche.cpp: 目标的源文件。
//

#include "demo.h"
#include "log_define.hpp"


int main(int argc, char* argv[])
{
	
	ltd_wapper ltd("./cache_data");
	std::vector<uint32_t> trading_days;
	// 查询某段时间的交易日
	ltd.get_daily_info(trading_days, "CFFEX.IC2501",20250101U,20250114U);
	for(auto& it : trading_days)
	{
		std::vector<ltd_tick_info> result;
		ltd.get_history_tick(result, "CFFEX.IC2501", it);
		for (const auto& it : result)
		{
			LOG_INFO('\t', it.code,'\t', it.trading_day, '\t', it.time, '\t', it.price);
		}
		std::this_thread::sleep_for(std::chrono::seconds(100));
	}
	
	return 0;
}
