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
#include <fstream>
#include <runtime_engine.h>
#include <time_utils.hpp>
#include <interface.h>
#include <filesystem>
#include <inipp.h>

using namespace lt::hft;

runtime_engine::runtime_engine(const char* config_path):engine(), _trader(nullptr), _market(nullptr)
{
	if (!std::filesystem::exists(config_path))
	{
		LOG_ERROR("runtime_engine init_from_file config_path not exit : %s", config_path);
		return ;
	}
	inipp::Ini<char> ini;
	std::ifstream is(config_path);
	ini.parse(is);
	auto it = ini.sections.find("include");
	if (it == ini.sections.end())
	{
		LOG_ERROR("runtime_engine init_from_file cant find [include]", config_path);
		return ;
	}
	params include_patams(it->second);
	it = ini.sections.find("actual_market");
	if (it == ini.sections.end())
	{
		LOG_ERROR("runtime_engine init_from_file cant find [actual_market]", config_path);
		return ;
	}
	//market
	_market = create_actual_market(it->second);
	if (_market == nullptr)
	{
		LOG_ERROR("runtime_engine init_from_file create_market_api ", config_path);
		return ;
	}
	it = ini.sections.find("actual_trader");
	if (it == ini.sections.end())
	{
		LOG_ERROR("runtime_engine init_from_file cant find [actual_trader]", config_path);
		return ;
	}
	//trader
	_trader = create_actual_trader(it->second);
	if (_trader == nullptr)
	{
		LOG_ERROR("runtime_engine init_from_file create_trader_api error : %s", config_path);
		return ;
	}
	it = ini.sections.find("control");
	if (it == ini.sections.end())
	{
		LOG_ERROR("runtime_engine init_from_filecant find [control]", config_path);
		return ;
	}
	params control_patams(it->second);
	this->_ctx.init(control_patams, include_patams, _market, _trader);
}
runtime_engine::~runtime_engine()
{
	if (_market)
	{
		destory_actual_market(_market);
	}
	if (_trader)
	{
		destory_actual_trader(_trader);
	}
}


void runtime_engine::start_trading(const std::vector<std::shared_ptr<lt::hft::strategy>>& strategies)
{
	if (_trader && _trader->login())
	{
		if (_market && _market->login())
		{
			this->regist_strategy(strategies);
			if(_ctx.start_service())
			{
				LOG_INFO("runtime_engine run in start_trading");
			}
		}
	}
}

void runtime_engine::stop_trading()
{
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	if (_ctx.stop_service())
	{
		clear_strategy();
		if (_trader)
		{
			_trader->logout();
		}
		if (_market)
		{
			_market->logout();
		}
		LOG_INFO("runtime_engine run end");
	}
}
