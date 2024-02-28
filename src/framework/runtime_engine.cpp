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
