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
#include <filesystem>
#include <basic_define.h>
#include "engine.hpp"
#include <interface.h>
#include <inipp.h>

namespace lt
{
	class actual_market;

	class actual_trader;
}

namespace lt::hft
{
	class runtime : public engine
	{

		actual_market* _market;

		actual_trader* _trader;

	public:

		runtime(const char* account_config, const char* control_config, const char* section_config) :
			engine(control_config), _trader(nullptr), _market(nullptr)
		{
			if (!std::filesystem::exists(account_config))
			{
				PRINT_ERROR("runtime_engine init_from_file config_path not exit : %s", account_config);
				return;
			}
			inipp::Ini<char> ini;
			std::ifstream is(account_config);
			ini.parse(is);

			auto it = ini.sections.find("actual_market");
			if (it == ini.sections.end())
			{
				PRINT_ERROR("runtime_engine init_from_file cant find [actual_market]", account_config);
				return;
			}
			//market
			_market = create_actual_market(it->second);
			if (_market == nullptr)
			{
				PRINT_ERROR("runtime_engine init_from_file create_market_api ", account_config);
				return;
			}
			it = ini.sections.find("actual_trader");
			if (it == ini.sections.end())
			{
				PRINT_ERROR("runtime_engine init_from_file cant find [actual_trader]", account_config);
				return;
			}
			//trader
			_trader = create_actual_trader(it->second);
			if (_trader == nullptr)
			{
				PRINT_ERROR("runtime_engine init_from_file create_trader_api error : %s", account_config);
				return;
			}
			this->_ctx = new trading_context(_market, _trader, section_config);
		}

		virtual ~runtime()
		{
			if (this->_ctx)
			{
				delete this->_ctx;
				this->_ctx = nullptr;
			}
			if (_market)
			{
				destory_actual_market(_market);
			}
			if (_trader)
			{
				destory_actual_trader(_trader);
			}
		}

	public:

		void start_trading(const std::vector <std::shared_ptr<lt::hft::strategy>>& strategys)
		{
			if (_trader && _trader->login())
			{
				if (_market && _market->login())
				{
					this->regist_strategy(strategys);
					if (this->start_service())
					{
						PRINT_INFO("runtime_engine run in start_trading");
					}
				}
			}
		}

		void stop_trading()
		{
			//std::this_thread::sleep_for(std::chrono::seconds(1));
			if (this->stop_service())
			{
				this->clear_strategy();
				if (_trader)
				{
					_trader->logout();
				}
				if (_market)
				{
					_market->logout();
				}
				PRINT_INFO("runtime_engine run end");
			}
		}

	};
}


