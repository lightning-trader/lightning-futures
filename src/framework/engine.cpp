#include "engine.h"

using namespace lt;

void subscriber::regist_tick_receiver(const code_t& code, tick_receiver* receiver)
{
	auto it = _tick_receiver.find(code);
	if (it == _tick_receiver.end())
	{
		_tick_receiver[code].insert(receiver);
	}
	else
	{
		it->second.insert(receiver);
	}
	auto d_it = tick_subscrib.find(code);
	if (d_it == tick_subscrib.end())
	{
		tick_subscrib.insert(code);
	}

}

void unsubscriber::unregist_tick_receiver(const code_t& code, tick_receiver* receiver)
{
	auto it = _tick_receiver.find(code);
	if (it == _tick_receiver.end())
	{
		return;
	}
	auto s_it = it->second.find(receiver);
	if (s_it != it->second.end())
	{
		it->second.erase(s_it);
	}
	if (it->second.empty())
	{
		_tick_receiver.erase(it);
	}
	auto d_it = tick_unsubscrib.find(code);
	if (d_it == tick_unsubscrib.end())
	{
		tick_unsubscrib.insert(code);
	}
}

void subscriber::regist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver)
{
	auto it = _bar_receiver.find(code);
	if (it == _bar_receiver.end())
	{
		_bar_receiver[code].insert(std::make_pair(period, receiver));
	}
	else
	{
		it->second.insert(std::make_pair(period, receiver));
	}
	auto ts_it = tick_subscrib.find(code);
	if (ts_it == tick_subscrib.end())
	{
		tick_subscrib.insert(code);
	}
	auto bs_it = bar_subscrib[code].find(period);
	if (bs_it == bar_subscrib[code].end())
	{
		bar_subscrib[code].insert(period);
	}
}

void unsubscriber::unregist_bar_receiver(const code_t& code, uint32_t period)
{
	auto it = _bar_receiver.find(code);
	if (it == _bar_receiver.end())
	{
		return;
	}
	auto s_it = it->second.find(period);
	if (s_it != it->second.end())
	{
		it->second.erase(s_it);
	}
	if (it->second.empty())
	{
		_bar_receiver.erase(it);
	}
	auto b_it = bar_unsubscrib.find(code);
	if(b_it == bar_unsubscrib.end())
	{
		bar_unsubscrib[code].insert(period);
	}else
	{
		auto p_it = bar_unsubscrib[code].find(period);
		if(p_it == bar_unsubscrib[code].end())
		{
			bar_unsubscrib[code].insert(period);
		}
	}
	
	//如果是最后一个receiver，去掉这个bar已经没有用了，那么把recever也要删掉
	auto tr_it = _tick_receiver.find(code);
	if (tr_it == _tick_receiver.end())
	{
		auto d_it = tick_unsubscrib.find(code);
		if (d_it == tick_unsubscrib.end())
		{
			tick_unsubscrib.insert(code);
		}
	}
}

engine::engine(context_type ctx_type,const char* config_path)
{
	_lt = lt_create_context(ctx_type, config_path);
	engine::_self = this;
	lt_bind_realtime_event(_lt, order_event{ _realtime_entrust_callback ,_realtime_deal_callback ,_realtime_trade_callback ,_realtime_cancel_callback ,_realtime_error_callback }, _ready_callback,_update_callback);
}

engine::~engine()
{
	lt_destory_context(_lt);
}


void engine::regist_strategy(const std::vector<std::shared_ptr<lt::strategy>>& strategys)
{
	subscriber suber(_tick_receiver, _bar_receiver);
	for (auto it : strategys)
	{
		it->init(suber);
		_strategy_map[it->get_id()] = (it);
	}
	subscribe(suber.tick_subscrib, suber.bar_subscrib);
}
void engine::clear_strategy()
{
	//策略不存在了那么订单和策略的映射关系也要清掉
	_estid_to_strategy.clear();
	unsubscriber unsuber(_tick_receiver, _bar_receiver);
	for (auto it : _strategy_map)
	{
		it.second->destroy(unsuber);
	}
	_strategy_map.clear();
	unsubscribe(unsuber.tick_unsubscrib, unsuber.bar_unsubscrib);
	
}


void engine::regist_estid_strategy(estid_t estid, straid_t straid)
{
	_estid_to_strategy[estid] = straid;
}
void engine::unregist_estid_strategy(estid_t estid)
{
	auto it = _estid_to_strategy.find(estid);
	if (it != _estid_to_strategy.end())
	{
		_estid_to_strategy.erase(it);
	}
}


void engine::set_trading_filter(filter_function callback)
{
	engine::_filter_function = callback;
	lt_set_trading_filter(_lt, engine::_filter_callback);
}


const order_statistic& lt::engine::get_order_statistic()const
{
	return lt_get_order_statistic(_lt);
}


estid_t engine::place_order(untid_t id,offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	PROFILE_DEBUG(code.get_id());
	LOG_INFO("place_order : ", code.get_id(), offset, direction, price);
	estid_t estid = lt_place_order(_lt, id, offset, direction, code, count, price, flag);
	if (estid != INVALID_ESTID)
	{
		regist_estid_strategy(estid, id);
	}
	return estid;
}


void engine::cancel_order(estid_t order_id)
{
	LOG_DEBUG("cancel_order : ", order_id);

	lt_cancel_order(_lt, order_id);
}


const position_info& engine::get_position(const code_t& code) const
{
	return lt_get_position(_lt, code);
}

const account_info& engine::get_account() const
{
	return lt_get_account(_lt);
}

const order_info& engine::get_order(estid_t order_id) const
{
	return lt_get_order(_lt, order_id);
}


daytm_t engine::get_last_time() const
{
	return lt_get_last_time(_lt);
}

daytm_t engine::get_close_time() const
{
	return lt_get_close_time(_lt);
}

daytm_t engine::next_open_time(daytm_t time) const
{
	return lt_next_open_time(_lt, time);
}

bool engine::is_in_trading(daytm_t time) const
{
	return lt_is_in_trading(_lt, time);
}

void engine::use_custom_chain(untid_t id,bool flag)
{
	lt_use_custom_chain(_lt, id, flag);
}

void engine::set_cancel_condition(estid_t order_id, std::function<bool()> callback)
{
	LOG_DEBUG("set_cancel_condition : ", order_id);
	engine::_condition_function[order_id] = callback;
	lt_set_cancel_condition(_lt, order_id, engine::_condition_callback);
}



daytm_t engine::last_order_time()
{
	return lt_last_order_time(_lt);
}


uint32_t engine::get_trading_day()const
{
	return lt_get_trading_day(_lt);
}

bool engine::is_trading_ready()const
{
	return lt_is_trading_ready(_lt);
}

const today_market_info& engine::get_today_market_info(const code_t& code)const
{
	return lt_get_today_market_info(_lt, code);
}
uint32_t engine::get_pending_position(const code_t& code,offset_type offset, direction_type direction)const
{
	return lt_get_pending_position(_lt, code, offset, direction);
}

void engine::bind_delayed_notify(std::shared_ptr<notify> notify)
{
	_all_notify.emplace_back(notify);
}

void engine::subscribe(const std::set<code_t>& tick_data, const std::map<code_t, std::set<uint32_t>>& bar_data)
{
	return lt_subscribe(_lt, tick_data, _tick_callback, bar_data, _bar_callback);
}

void engine::unsubscribe(const std::set<code_t>& tick_data, const std::map<code_t, std::set<uint32_t>>& bar_data)
{
	return lt_unsubscribe(_lt, tick_data, bar_data);
}