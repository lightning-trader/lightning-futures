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
#include "price_step.h"
#include <define_types.hpp>
#include <rapidcsv.h>
#include "log_wapper.hpp"
#include <time_utils.hpp>

price_step::price_step(const std::string& config_path)
{
	LOG_INFO("price_step init ");
	_price_step_data.clear();
	rapidcsv::Document config_csv(config_path, rapidcsv::LabelParams(0, -1));
	for (size_t i = 0; i < config_csv.GetRowCount(); i++)
	{
		const std::string& code_str = config_csv.GetCell<std::string>("code", i);
		if(!code_str.empty())
		{
			double_t price_step = config_csv.GetCell<double_t>("price_step", i);
			_price_step_data[code_str.c_str()] = price_step;
		}
	}
}
price_step::~price_step()
{

}

double_t price_step::get_price_step(const code_t& code)const
{
	auto it = _price_step_data.find(code);
	if(it != _price_step_data.end())
	{
		return it->second;
	}
	return 1.0;
}

double_t price_step::get_proximate_price(const code_t& code, double_t row_price)const
{
	auto step = get_price_step(code);
	return std::round( row_price / step ) * step;
}