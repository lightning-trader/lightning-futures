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
#include "csv_recorder.h"
#include <filesystem>
#include <time_utils.hpp>
#include <log_wapper.hpp>

using namespace lt::hft;

csv_recorder::csv_recorder(const char* basic_path) :_crossday_flow_csv(std::string(),rapidcsv::LabelParams(0,-1))
{
	if (!std::filesystem::exists(basic_path))
	{
		std::filesystem::create_directories(basic_path);
	}
	time_t now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	_basic_path = std::string(basic_path)+"/"+datetime_to_string(now_time,"%Y-%m-%d");
	if (!std::filesystem::exists(_basic_path.c_str()))
	{
		std::filesystem::create_directories(_basic_path.c_str());
	}

	_crossday_flow_csv.SetColumnName(0, "trading_day");
	_crossday_flow_csv.SetColumnName(1, "place_order_amount");
	_crossday_flow_csv.SetColumnName(2, "entrust_amount");
	_crossday_flow_csv.SetColumnName(3, "trade_amount");
	_crossday_flow_csv.SetColumnName(4, "cancel_amount");
	_crossday_flow_csv.SetColumnName(5, "error_amount");
	_crossday_flow_csv.SetColumnName(6, "money");
	_crossday_flow_csv.SetColumnName(7, "frozen");
}

//结算表
void csv_recorder::record_crossday_flow(uint32_t trading_day, const order_statistic& statistic, const account_info& account)
{
	try
	{
		size_t count = _crossday_flow_csv.GetRowCount();
		std::vector<std::string> row_data;
		row_data.emplace_back(std::to_string(trading_day));
		row_data.emplace_back(std::to_string(statistic.place_order_amount));
		row_data.emplace_back(std::to_string(statistic.entrust_amount));
		row_data.emplace_back(std::to_string(statistic.trade_amount));
		row_data.emplace_back(std::to_string(statistic.cancel_amount));
		row_data.emplace_back(std::to_string(statistic.error_amount));
		row_data.emplace_back(std::to_string(account.money));
		row_data.emplace_back(std::to_string(account.frozen_monery));
		_crossday_flow_csv.InsertRow<std::string>(count, row_data);
		_crossday_flow_csv.Save(_basic_path + "/crossday_flow.csv");
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("csv_recorder record_crossday_flow exception : ", e.what());
	}
}