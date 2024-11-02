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
#include "evaluate_engine.h"
#include "csv_recorder.h"
#include <filesystem>
#include <inipp.h>
#include <thread>
#include <interface.h>
#include <market_api.h>
#include <trader_api.h>

using namespace lt::hft;

evaluate_engine::evaluate_engine(const char* config_path):engine(), _market_simulator(nullptr), _trader_simulator(nullptr)
{
	if (!std::filesystem::exists(config_path))
	{
		LOG_ERROR("evaluate_engine init_from_file config_path not exit : %s", config_path);
		return ;
	}
	inipp::Ini<char> ini;
	std::ifstream is(config_path);
	ini.parse(is);

	auto it = ini.sections.find("include");
	if (it == ini.sections.end())
	{
		LOG_ERROR("evaluate_engine cant find [include]", config_path);
		return ;
	}
	params include_patams(it->second);
	it = ini.sections.find("dummy_market");
	if (it == ini.sections.end())
	{
		LOG_ERROR("evaluate_engine cant find [dummy_market]", config_path);
		return ;
	}
	_market_simulator = create_dummy_market(it->second);
	if (_market_simulator == nullptr)
	{
		LOG_ERROR("evaluate_engine create_dummy_market error : %s", config_path);
		return ;
	}
	it = ini.sections.find("dummy_trader");
	if (it == ini.sections.end())
	{
		LOG_ERROR("evaluate_engine cant find [dummy_trader]", config_path);
		return ;
	}
	_trader_simulator = create_dummy_trader(it->second);
	if (_trader_simulator == nullptr)
	{
		LOG_ERROR("evaluate_engine create_dummy_trader error : %s", config_path);
		return ;
	}
	it = ini.sections.find("recorder");
	if (it != ini.sections.end())
	{
		params recorder_patams(it->second);
		const auto& recorder_path = recorder_patams.get<std::string>("basic_path");
		_recorder = std::make_shared<csv_recorder>(recorder_path.c_str());
	}
	it = ini.sections.find("control");
	if (it == ini.sections.end())
	{
		LOG_ERROR("evaluate_engine cant find [control]", config_path);
		return ;
	}
	params control_patams(it->second);
	this->_ctx.init(control_patams, include_patams, _market_simulator, _trader_simulator, true);
}
evaluate_engine::~evaluate_engine()
{
	if (_market_simulator)
	{
		destory_dummy_market(_market_simulator);
	}
	if (_trader_simulator)
	{
		destory_dummy_trader(_trader_simulator);
	}
}

void evaluate_engine::back_test(const std::vector<std::shared_ptr<lt::hft::strategy>>& strategies, uint32_t trading_day)
{
	if (!_trader_simulator)
	{
	}
	_trader_simulator->crossday(trading_day);
	this->regist_strategy(strategies);
	if(this->_ctx.start_service())
	{
		_market_simulator->play(_trader_simulator->get_trading_day(), [this](const std::vector<const tick_info*>& current_tick)->void {
			_trader_simulator->push_tick(current_tick);
			});
		while (!_market_simulator->is_finished())
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		rapidcsv::Document _crossday_flow_csv;
		//记录结算数据
		if (_recorder)
		{
			_recorder->record_crossday_flow(_trader_simulator->get_trading_day(), _ctx.get_all_statistic(), _trader_simulator->get_account());
		}
		if(this->_ctx.stop_service())
		{
			this->clear_strategy();
		}
	}
}
