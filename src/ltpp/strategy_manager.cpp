#include "strategy_manager.h"

void strategy_manager::regist_strategy(straid_t straid, std::shared_ptr<strategy> stra)
{
	if(stra)
	{
		stra->init(straid,this);
		_strategy_map[straid] = (stra);
	}
}
void strategy_manager::unregist_strategy(straid_t straid)
{
	auto it = _strategy_map.find(straid);
	if(it != _strategy_map.end())
	{
		it->second->on_destory();
		_strategy_map.erase(it);
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
