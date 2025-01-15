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
#include "contract_parser.h"
#include <log_define.hpp>


using namespace lt::driver;

contract_parser::contract_parser()
{

}
contract_parser::~contract_parser()
{
	
}

void contract_parser::init(const std::string& config_path)
{
	LOG_INFO("contract_parser init");
	_contract_info.clear();
	rapidcsv::Document config_csv(config_path, rapidcsv::LabelParams(0, -1));
	for (size_t i = 0; i < config_csv.GetRowCount(); i++)
	{
		contract_info info ;
		const std::string& code_str = config_csv.GetCell<std::string>("code", i);
		LOG_INFO("load contract code :", code_str.c_str());
		info.code = code_t(code_str.c_str());
		info.crge_type = static_cast<charge_type>(config_csv.GetCell<int32_t>("charge_type", i));
		info.open_charge = config_csv.GetCell<double_t>("open_charge", i);
		info.close_today_charge = config_csv.GetCell<double_t>("close_today_charge", i);
		info.close_yestoday_charge = config_csv.GetCell<double_t>("close_yestoday_charge", i);
	
		info.multiple = config_csv.GetCell<double_t>("multiple", i);
		info.margin_rate = config_csv.GetCell<double_t>("margin_rate", i);
		_contract_info[info.code] = info;
		
	}
}
const contract_info* contract_parser::get_contract_info(const code_t& code)const
{
	auto it = _contract_info.find(code);
	if(it != _contract_info.end())
	{
		return &(it->second);
 	}
	return nullptr;
}