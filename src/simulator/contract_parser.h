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
#pragma once
#include <rapidcsv.h>
#include <define.h>
#include <define_types.hpp>

enum class charge_type
{
	CT_FIXED_AMOUNT = 1 ,//固定金额
	CT_PRICE_RATIO = 2,//价格比例
};

struct contract_info
{
	code_t code ;
	charge_type crge_type;
	double_t open_charge;
	double_t close_today_charge;
	double_t close_yestoday_charge;

	double_t multiple; //乘数
	double_t margin_rate; //保证金率


	contract_info():crge_type(charge_type::CT_FIXED_AMOUNT), open_charge(.0F), close_today_charge(.0F), close_yestoday_charge(.0F), multiple(.0F), margin_rate(.0F) {}

	inline double_t get_service_charge(double_t price, offset_type offset)const
	{
		if(crge_type == charge_type::CT_FIXED_AMOUNT)
		{
			if(offset == offset_type::OT_OPEN)
			{
				return open_charge;
			}
			else if (offset == offset_type::OT_CLSTD)
			{
				return close_today_charge;
			}
			else
			{
				return close_yestoday_charge;
			}
		}
		if (crge_type == charge_type::CT_PRICE_RATIO)
		{
			if (offset == offset_type::OT_OPEN)
			{
				return open_charge * price * multiple;
			}
			else if (offset == offset_type::OT_CLSTD)
			{
				return close_today_charge * price * multiple;
			}
			else
			{
				return close_yestoday_charge * price * multiple;
			}
		}
		return .0F;
	}
};

class contract_parser
{

public:
	contract_parser();
	virtual ~contract_parser();

private:

	std::map<code_t, contract_info> _contract_info;
public:

	void init(const std::string& config_path);

	const contract_info* get_contract_info(const code_t&)const;

};

