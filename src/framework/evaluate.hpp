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
#include "strategy.hpp"
#include <time_utils.hpp>
#include <rapidcsv.h>
#include <shared_types.h>
#include <inipp.h>
#include <interface.h>

namespace lt
{

	class dummy_market;

	class dummy_trader;
}

namespace lt::hft
{
	class csv_recorder
	{
	private:

		std::string _basic_path;

		rapidcsv::Document _crossday_flow_csv;

	public:

		csv_recorder(const char* basic_path)
		{
			if (!std::filesystem::exists(basic_path))
			{
				std::filesystem::create_directories(basic_path);
			}
			time_t now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			_basic_path = std::string(basic_path) + "/" + lt::datetime_to_string(now_time, "%Y-%m-%d");
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

	public:

		//结算表
		void record_crossday_flow(uint32_t trading_day, const order_statistic& statistic, const account_info& account)
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
				LTLOG_ERROR("csv_recorder record_crossday_flow exception : ", e.what());
			}
		}

	};
	class evaluate : public engine
	{

		dummy_market* _market_simulator;

		dummy_trader* _trader_simulator;

		std::unique_ptr<class csv_recorder> _recorder;


	public:

		evaluate(const char* account_config, const char* control_config, const char* section_config) :engine(control_config), _market_simulator(nullptr), _trader_simulator(nullptr)
		{
			if (!std::filesystem::exists(account_config))
			{
				LTLOG_FATAL("evaluate_engine init_from_file config_path not exists : %s", account_config);
				return;
			}
			inipp::Ini<char> ini;
			std::ifstream is(account_config);
			ini.parse(is);


			auto it = ini.sections.find("dummy_market");
			if (it == ini.sections.end())
			{
				LTLOG_ERROR("evaluate_engine cant find [dummy_market]", account_config);
				return;
			}
			_market_simulator = create_dummy_market(it->second);
			if (_market_simulator == nullptr)
			{
				LTLOG_ERROR("evaluate_engine create_dummy_market error : %s", account_config);
				return;
			}
			it = ini.sections.find("dummy_trader");
			if (it == ini.sections.end())
			{
				LTLOG_ERROR("evaluate_engine cant find [dummy_trader]", account_config);
				return;
			}
			_trader_simulator = create_dummy_trader(it->second);
			if (_trader_simulator == nullptr)
			{
				LTLOG_ERROR("evaluate_engine create_dummy_trader error : %s", account_config);
				return;
			}
			it = ini.sections.find("recorder");
			if (it != ini.sections.end())
			{
				params recorder_patams(it->second);
				const auto& recorder_path = recorder_patams.get<std::string>("basic_path");
				_recorder = std::make_unique<csv_recorder>(recorder_path.c_str());
			}
			this->_ctx = new trading_context(_market_simulator, _trader_simulator, section_config, true);
		}
		
		virtual ~evaluate()
		{
			if (this->_ctx)
			{
				delete this->_ctx;
				this->_ctx = nullptr;
			}
			if (_market_simulator)
			{
				destory_dummy_market(_market_simulator);
			}
			if (_trader_simulator)
			{
				destory_dummy_trader(_trader_simulator);
			}
		}


	public:

		void back_test(const std::vector<std::shared_ptr<lt::hft::strategy>>& strategys, uint32_t begin_day, uint32_t end_day)
		{
			this->regist_strategy(strategys);
			if (this->start_service())
			{
				_market_simulator->set_trading_range(begin_day, end_day);
				_market_simulator->set_publish_callback([this](const std::vector<const tick_info*>& current_tick)->void {
					_trader_simulator->push_tick(current_tick);
					});
				_market_simulator->set_crossday_callback([this](uint32_t form, uint32_t to)->void {
					_trader_simulator->crossday(to);
					});

				if (_market_simulator->play())
				{
					while (!_market_simulator->is_finished())
					{
						std::this_thread::sleep_for(std::chrono::seconds(1));
					}
					rapidcsv::Document _crossday_flow_csv;
					//记录结算数据
					if (_recorder)
					{
						_recorder->record_crossday_flow(_trader_simulator->get_trading_day(), _ctx->get_all_statistic(), _trader_simulator->get_account());
					}
				}


				if (this->stop_service())
				{
					this->clear_strategy();
				}
			}

		}

	};


}
