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
#include <filesystem>
#include "runtime.h"
#include <define.h>
#include <interface.h>
#include "inipp.h"


runtime::runtime():_market(nullptr), _trader(nullptr)
{
}
runtime::~runtime()
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

bool runtime::init_from_file(const std::string& config_path)
{
	if (!std::filesystem::exists(config_path.c_str()))
	{
		LOG_ERROR("runtime_engine init_from_file config_path not exit : %s", config_path.c_str());
		return false;
	}
	inipp::Ini<char> ini;
	std::ifstream is(config_path.c_str());
	ini.parse(is);
	auto it = ini.sections.find("include");
	if (it == ini.sections.end())
	{
		LOG_ERROR("runtime_engine init_from_file cant find [include]", config_path.c_str());
		return false;
	}
	params include_patams(it->second);
	it = ini.sections.find("actual_market");
	if (it == ini.sections.end())
	{
		LOG_ERROR("runtime_engine init_from_file cant find [actual_market]", config_path.c_str());
		return false;
	}
	auto&& id_excg_map = std::make_shared<std::unordered_map<std::string,std::string>>();
	//market
	_market = create_actual_market(id_excg_map,it->second);
	if (_market == nullptr)
	{
		LOG_ERROR("runtime_engine init_from_file create_market_api ", config_path.c_str());
		return false;
	}
	it = ini.sections.find("actual_trader");
	if (it == ini.sections.end())
	{
		LOG_ERROR("runtime_engine init_from_file cant find [actual_trader]", config_path.c_str());
		return false;
	}
	//trader
	_trader = create_actual_trader(id_excg_map,it->second);
	if (_trader == nullptr)
	{
		LOG_ERROR("runtime_engine init_from_file create_trader_api error : %s", config_path.c_str());
		return false;
	}
	it = ini.sections.find("control");
	if (it == ini.sections.end())
	{
		LOG_ERROR("runtime_engine init_from_filecant find [control]", config_path.c_str());
		return false;
	}
	params control_patams(it->second);
	this->init(control_patams, include_patams);
	return true;
}


bool runtime::login_account()
{
	if (_trader && _trader->login())
	{
		if (_market && _market->login())
		{
			return true;
		}
	}
	return false ;
}

void runtime::logout_account()
{
	if (_trader)
	{
		_trader->logout();
	}
	if (_market)
	{
		_market->logout();
	}
}

trader_api& runtime::get_trader()
{
	return *_trader;
}

market_api& runtime::get_market()
{
	return *_market;
}

void runtime::on_update()
{
	
}

bool runtime::is_terminaled()
{
	if (_trader)
	{
		return _trader->is_idle();
	}
	return false;
}
