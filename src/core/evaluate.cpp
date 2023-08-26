#include "evaluate.h"
#include <filesystem>
#include "context.h"
#include "csv_recorder.h"
#include <market_api.h>
#include <interface.h>
#include "inipp.h"
#include <params.hpp>

evaluate::evaluate():_market_simulator(nullptr), _trader_simulator(nullptr)
{
}
evaluate::~evaluate()
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

bool evaluate::init_from_file(const std::string& config_path)
{
	
	if (!std::filesystem::exists(config_path.c_str()))
	{
		LOG_ERROR("evaluate_driver init_from_file config_path not exit : %s", config_path.c_str());
		return false;
	}
	inipp::Ini<char> ini;
	std::ifstream is(config_path.c_str());
	ini.parse(is);
	
	auto it = ini.sections.find("include");
	if (it == ini.sections.end())
	{
		LOG_ERROR("init_from_file cant find [include]", config_path.c_str());
		return false;
	}
	params include_patams(it->second);
	it = ini.sections.find("dummy_market");
	if (it == ini.sections.end())
	{
		LOG_ERROR("init_from_file cant find [dummy_market]", config_path.c_str());
		return false;
	}
	_market_simulator = create_dummy_market(it->second);
	if (_market_simulator == nullptr)
	{
		LOG_ERROR("init_from_file create_dummy_market error : %s", config_path.c_str());
		return false;
	}
	it = ini.sections.find("dummy_trader");
	if (it == ini.sections.end())
	{
		LOG_ERROR("init_from_file cant find [dummy_trader]", config_path.c_str());
		return false;
	}
	_trader_simulator = create_dummy_trader(it->second);
	if (_trader_simulator == nullptr)
	{
		LOG_ERROR("init_from_file create_dummy_trader error : %s", config_path.c_str());
		return false;
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
		LOG_ERROR("init_from_file cant find [control]", config_path.c_str());
		return false;
	}
	params control_patams(it->second);
	this->init(control_patams, include_patams,true);
	return true;
}

void evaluate::playback_history()
{
	
	if(_market_simulator)
	{	
		_market_simulator->play(_trader_simulator->get_trading_day(), [this](const tick_info& tick)->void {
			_trader_simulator->push_tick(tick);
		});
		rapidcsv::Document _crossday_flow_csv;
		//记录结算数据
		if (_recorder)
		{
			_recorder->record_crossday_flow(_trader_simulator->get_trading_day(), get_order_statistic(), get_account());
		}
	}
}

void evaluate::simulate_crossday(uint32_t trading_day)
{
	if(_trader_simulator)
	{
		_trader_simulator->crossday(trading_day);
	}
}

trader_api& evaluate::get_trader()
{
	return *_trader_simulator;
}

market_api& evaluate::get_market()
{
	return *_market_simulator;
}

void evaluate::on_update()
{
	if(_market_simulator)
	{
		_market_simulator->update();
	}
	if (is_in_trading()&& _trader_simulator)
	{
		while (!_trader_simulator->is_empty())
		{
			_trader_simulator->update();
		}
	}
}

bool evaluate::is_terminaled()
{
	if(_trader_simulator)
	{
		return _trader_simulator->is_empty();
	}
	return false;
}

void evaluate::add_market_handle(std::function<void(market_event_type, const std::vector<std::any>&)> handle)
{
	if (_market_simulator)
	{
		_market_simulator->add_handle(handle);
	}
}

void evaluate::add_trader_handle(std::function<void(trader_event_type, const std::vector<std::any>&)> handle)
{
	if (_trader_simulator)
	{
		_trader_simulator->add_handle(handle);
	}
}