#include "runtime.h"
#include <define.h>
#include <filesystem>
#include <market_api.h>
#include <trader_api.h>
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


void runtime::login_account()
{
	if (_trader)
	{
		_trader->login();
	}
	if (_market)
	{
		_market->login();
	}
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
	if (_market)
	{
		_market->update();
	}
	if (is_in_trading() && _trader)
	{
		while (!_trader->is_empty())
		{
			_trader->update();
		}
	}
}

bool runtime::is_terminaled()
{
	if (_trader)
	{
		return _trader->is_empty();
	}
	return false;
}

void runtime::add_market_handle(std::function<void(market_event_type, const std::vector<std::any>&)> handle)
{
	if(_market)
	{
		_market->add_handle(handle);
	}
}

void runtime::add_trader_handle(std::function<void(trader_event_type, const std::vector<std::any>&)> handle)
{
	if (_trader)
	{
		_trader->add_handle(handle);
	}
}
