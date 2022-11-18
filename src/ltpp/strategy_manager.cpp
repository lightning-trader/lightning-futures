#include "strategy_manager.h"

void strategy_manager::regist_strategy(std::shared_ptr<strategy> stra)
{
	if(stra)
	{
		stra->init(this);
		_strategy_list.emplace_back(stra);
	}
}
void strategy_manager::unregist_strategy(std::shared_ptr<strategy> stra)
{
	auto it = std::find(_strategy_list.begin(), _strategy_list.end(), stra);
	if(it != _strategy_list.end())
	{
		_strategy_list.erase(it);
	}
	
}

void strategy_manager::regist_code_strategy(const code_t& code, strategy* stra)
{
	_code_to_strategy[code].insert(stra);
}

void strategy_manager::unregist_code_strategy(const code_t& code, strategy* stra)
{
	auto it = _code_to_strategy.find(code);
	if(it == _code_to_strategy.end())
	{
		return ;
	}
	auto s_it = it->second.find(stra);
	if(s_it != it->second.end())
	{
		it->second.erase(s_it);
	}
	if(it->second.empty())
	{
		_code_to_strategy.erase(it);
	}
}

void strategy_manager::regist_estid_strategy(estid_t estid, strategy* stra)
{
	_estid_to_strategy[estid] = stra;
}
void strategy_manager::unregist_estid_strategy(estid_t estid)
{
	auto it = _estid_to_strategy.find(estid);
	if(it != _estid_to_strategy.end())
	{
		_estid_to_strategy.erase(it);
	}
}
