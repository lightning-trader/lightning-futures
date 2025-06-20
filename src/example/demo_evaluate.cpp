﻿/*
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

#include <basic_define.h>
#include "evaluate.hpp"
#include <time_utils.hpp>
#include "marketing_strategy.h"
#include "orderflow_strategy.h"
#include "arbitrage_strategy.h"



int main(int argc, char* argv[])
{
	auto app = std::make_shared<lt::hft::evaluate>("config/evaluate_account.ini", "config/normal_control.ini", "config/alltrading_section.csv");
	std::vector<std::shared_ptr<lt::hft::strategy>> strategys;
	strategys.emplace_back(std::make_shared<marketing_strategy>(1, app.get(), "SHFE.rb2510", 1, 1));
	strategys.emplace_back(std::make_shared<orderflow_strategy>(2, app.get(), "SHFE.rb2210", 1, 1, 3, 3, 10));
	app->back_test(strategys, 20250618U,20250619U);
	return 0;
}
