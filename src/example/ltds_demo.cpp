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
#include <log_define.hpp>
#include <data_wapper.hpp>
#include <time_utils.hpp>

data_wapper ltd("ltds-release", "./cache");

void print_trading_day(uint32_t begin, uint32_t end)
{
	std::vector<uint32_t> trading_days;
	ltd.get_trading_calendar(trading_days, begin, end);
	for (auto it : trading_days)
	{
		PRINT_INFO(it);
	}
}
void print_all_instrument(uint32_t trading_day)
{
	std::vector<std::string> all_instruments;
	ltd.get_all_instrument(all_instruments, trading_day);
	for (auto it : all_instruments)
	{
		PRINT_INFO(it);
	}
}

void print_history_tick(const char* code, uint32_t trading_day)
{
	std::vector<ltd_tick_info> result;
	ltd.get_history_tick(result, code, trading_day);
	for (const auto& it : result)
	{
		PRINT_INFO('\t', it.code, '\t', it.trading_day, '\t', it.time, '\t', it.price);
	}
}

void print_history_bar(const char* code)
{
	std::vector<ltd_bar_info> result;
	//打印最近30根1分钟K线
	ltd.get_history_bar(result, code, ltd_period::BC_1M, lt::make_seqtm(20250919U,"15:51:00.000"), 30);
	for (const auto& it : result)
	{
		PRINT_INFO(it.code, '\t', lt::seqtm_to_string(it.time), '\t', it.open, it.high, it.close, it.low, '\t', it.volume);
	}
}

int main(int argc, char* argv[])
{
	print_history_bar("SHFE.ag2512");
	std::this_thread::sleep_for(std::chrono::seconds(60));
	return 0;
}
