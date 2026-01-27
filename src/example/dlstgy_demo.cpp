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

#include <thread>
#include "runtime.hpp"
#include <time_utils.hpp>
#include "marketing_strategy.h"
#include "orderflow_strategy.h"
#include <crontab_scheduler.hpp>


int main(int argc, char* argv[])
{

	auto app = std::make_shared<lt::hft::runtime>("config/runtime_ctpdev.ini", "config/control_bindcore.ini", "config/section_alltrading.csv");
	lt::hft::strategy_creater sc("stgy-marketing");
	//支持一个或者多个策略同时运行
	app->regist_strategy({ sc.make_strategy(1, app.get(), "code=CFFEX.TF2509&open_detla=1&open_once=1") });
	//设置拦截器对下单频率增加限制
	app->set_trading_filter([app](const lt::code_t& code, lt::offset_type offset, lt::direction_type direction, uint32_t count, double_t price, lt::order_flag flag)->bool {
		auto now = app->get_last_time();
		auto last_order = app->last_order_time();
		if (now - last_order < 1000)
		{
			return false;
		}
		return true;
		});
	//启动交给manager管理，可以支持7*24持续运行不重启
	lt::crontab_scheduler manager;

	// 设置周一到周五为工作日
	manager.set_work_days({ 1,2,3,4,5 });
	//夜盘 从20:45:00启动运行6小时（02:45:00）结束
	manager.add_schedule("20:45:00", std::chrono::hours(6));
	//日盘 从08:45:00启动运行7小时（13:45:00）结束
	manager.add_schedule("08:45:00", std::chrono::hours(7));

	manager.set_callback([app](int index) {
		app->start_trading();
		},
		[app](int index) {
			app->stop_trading();
		});

	while (true)
	{
		manager.polling();
	}
	return 0;
}
