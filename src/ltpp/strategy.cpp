#include "strategy.h"
#include "lightning.h"
#include "strategy_manager.h"


std::vector<std::string> split_string(const std::string& str, char delim) {
	std::size_t previous = 0;
	std::size_t current = str.find(delim);
	std::vector<std::string> elems;
	while (current != std::string::npos) {
		if (current > previous) {
			elems.push_back(str.substr(previous, current - previous));
		}
		previous = current + 1;
		current = str.find(delim, previous);
	}
	if (previous != str.size()) {
		elems.push_back(str.substr(previous));
	}
	return elems;
}

std::map<std::string, std::string> parse_params(const std::string& param)
{
	std::map<std::string, std::string> result;
	auto param_pair = split_string(param, '&');
	for (auto it : param_pair)
	{
		auto param = split_string(it, '=');
		result[param[0]] = param[1];
	}
	return result;
}

strategy::param::param(const char * str)
{
	_param = parse_params(str);
}

strategy::strategy()
{
}
strategy::~strategy()
{
	
}

/*
	*	初始化
	*/
void strategy::init(straid_t id, strategy_manager* manager)
{
	_id = id;
	_manager = manager;
	this->on_init();
}

estid_t strategy::buy_for_open(const code_t& code,uint32_t count ,double_t price , order_flag flag )
{

	return place_order(OT_OPEN, DT_LONG, code, count, price, flag);
}

estid_t strategy::sell_for_close(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	return place_order(OT_CLOSE, DT_LONG, code, count, price, flag);
}

estid_t strategy::sell_for_open(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	return place_order(OT_OPEN, DT_SHORT, code, count, price, flag);
}

estid_t strategy::buy_for_close(const code_t& code, uint32_t count, double_t price , order_flag flag )
{
	return place_order(OT_CLOSE, DT_SHORT,code, count, price, flag);
}

estid_t strategy::place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	LOG_DEBUG("place_order : %s , %d, %d, %f\n", code.get_id(), offset, direction, price);
	if (_manager == nullptr)
	{
		LOG_ERROR("strategy place_order _manager is nullptr");
		return INVALID_ESTID;
	}
	estid_t estid = lt_place_order(_manager->get_lt(),get_id(), offset, direction, code, count, price, flag);
	if(estid != INVALID_ESTID)
	{
		_manager->regist_estid_strategy(estid,this);
	}
	return estid ;
}


void strategy::cancel_order(estid_t order_id)
{
	LOG_DEBUG("cancel_order : %llu\n", order_id);
	if (_manager == nullptr)
	{
		LOG_ERROR("strategy cancel_order _manager is nullptr");
		return;
	}
	lt_cancel_order(_manager->get_lt(), order_id);
}


const position_info& strategy::get_position(const code_t& code) const
{
	if (_manager == nullptr)
	{
		return default_position;
	}
	return lt_get_position(_manager->get_lt(),code);
}

const account_info& strategy::get_account() const
{
	if (_manager == nullptr)
	{
		return default_account;
	}
	return lt_get_account(_manager->get_lt());
}

const order_info& strategy::get_order(estid_t order_id) const
{
	if (_manager == nullptr)
	{
		return default_order;
	}
	return lt_get_order(_manager->get_lt(),order_id);
}

void strategy::subscribe(const code_t& code)
{
	if (_manager == nullptr)
	{
		return;
	}
	_manager->regist_code_strategy(code, this);
	lt_subscribe(_manager->get_lt(),code);
}

void strategy::unsubscribe(const code_t& code)
{
	if (_manager == nullptr)
	{
		return;
	}
	lt_unsubscribe(_manager->get_lt(), code);
	_manager->unregist_code_strategy(code, this);
}
time_t strategy::get_last_time() const
{
	if (_manager == nullptr)
	{
		return -1;
	}
	return lt_get_last_time(_manager->get_lt());
}

void strategy::use_custom_chain(bool flag)
{
	if (_manager == nullptr)
	{
		return ;
	}
	return lt_use_custom_chain(_manager->get_lt(),get_id(),flag);
}

void strategy::set_cancel_condition(estid_t order_id,std::function<bool(const tick_info&)> callback)
{
	LOG_DEBUG("set_cancel_condition : %llu\n", order_id);
	if (_manager == nullptr)
	{
		return;
	}
	strategy_manager::_condition_function[order_id] = callback;
	lt_set_cancel_condition(_manager->get_lt(), order_id, strategy_manager::_condition_callback);
}



time_t strategy::last_order_time()
{
	if (_manager == nullptr)
	{
		return -1;
	}
	return lt_last_order_time(_manager->get_lt());
}


void* strategy::get_userdata(size_t size)
{
	if (_manager == nullptr)
	{
		return nullptr;
	}
	return lt_get_userdata(_manager->get_lt(), _id, size);;
}

uint32_t strategy::get_trading_day()const
{
	if (_manager == nullptr)
	{
		return 0;
	}
	return lt_get_trading_day(_manager->get_lt());
}

bool strategy::is_trading_ready()const
{
	if (_manager == nullptr)
	{
		return false;
	}
	return lt_is_trading_ready(_manager->get_lt());
}
