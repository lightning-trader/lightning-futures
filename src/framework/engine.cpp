#include "engine.h"
#include "bar_generator.h"

using namespace lt;

void subscriber::regist_tick_receiver(const code_t& code, tick_receiver* receiver)
{
	auto it = _engine._tick_receiver.find(code);
	if (it == _engine._tick_receiver.end())
	{
		_engine._tick_receiver[code].insert(receiver);
	}
	else
	{
		it->second.insert(receiver);
	}
	_engine._tick_reference_count[code]++;
}

void unsubscriber::unregist_tick_receiver(const code_t& code, tick_receiver* receiver)
{
	auto it = _engine._tick_receiver.find(code);
	if (it == _engine._tick_receiver.end())
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
		_engine._tick_receiver.erase(it);
	}
	auto d_it = _engine._tick_reference_count.find(code);
	if (d_it != _engine._tick_reference_count.end())
	{
		if(d_it->second > 0)
		{
			d_it->second--;
		}
	}
}

void subscriber::regist_tape_receiver(const code_t& code, tape_receiver* receiver)
{
	auto it = _engine._tape_receiver.find(code);
	if (it == _engine._tape_receiver.end())
	{
		_engine._tape_receiver[code].insert(receiver);
	}
	else
	{
		it->second.insert(receiver);
	}
	_engine._tick_reference_count[code]++;
}

void unsubscriber::unregist_tape_receiver(const code_t& code, tape_receiver* receiver)
{
	auto it = _engine._tape_receiver.find(code);
	if (it == _engine._tape_receiver.end())
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
		_engine._tape_receiver.erase(it);
	}
	auto d_it = _engine._tick_reference_count.find(code);
	if (d_it != _engine._tick_reference_count.end())
	{
		if (d_it->second > 1)
		{
			d_it->second--;
		}
	}
}

void subscriber::regist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver)
{
	auto generator_iter = _engine._bar_generator[code].find(period);
	if(generator_iter == _engine._bar_generator[code].end())
	{
		_engine._bar_generator[code][period] = std::make_shared<bar_generator>(period, _engine._ps_config->get_price_step(code));
	}
	_engine._bar_generator[code][period]->add_receiver(receiver);
	_engine._tick_reference_count[code]++;
}

void unsubscriber::unregist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver)
{
	auto it = _engine._bar_generator.find(code);
	if (it == _engine._bar_generator.end())
	{
		return;
	}
	auto s_it = it->second.find(period);
	if (s_it == it->second.end())
	{
		return;
	}
	s_it->second->remove_receiver(receiver);
	if(s_it->second->invalid())
	{
		it->second.erase(s_it);
		if (it->second.empty())
		{
			_engine._bar_generator.erase(it);
		}
	}

	auto d_it = _engine._tick_reference_count.find(code);
	if (d_it != _engine._tick_reference_count.end())
	{
		if (d_it->second > 1)
		{
			d_it->second--;
		}
	}
}

engine::engine(context_type ctx_type,const char* config_path)
{
	_lt = lt_create_context(ctx_type, config_path);
	engine::_self = this;
	lt_bind_realtime_event(_lt, order_event{ _entrust_callback ,_deal_callback ,_trade_callback ,_cancel_callback ,_error_callback }, _ready_callback,_update_callback);
	auto section_config = lt_get_include_config(_lt,"section_config");
	_section_config = std::make_shared<trading_section>(section_config);
	const auto& ps_config = lt_get_include_config(_lt, "price_step_config");
	_ps_config = std::make_shared<price_step>(ps_config);
}

engine::~engine()
{
	lt_destory_context(_lt);
}


void engine::regist_strategy(const std::vector<std::shared_ptr<lt::strategy>>& strategys)
{
	subscriber suber(*this);
	for (auto it : strategys)
	{
		it->init(suber);
		_strategy_map[it->get_id()] = (it);
	}
	std::set<code_t> tick_subscrib;
	for (auto it = _tick_reference_count.begin(); it != _tick_reference_count.end();)
	{
		if(it->second == 0)
		{
			it = _tick_reference_count.erase(it);
		}
		else
		{
			tick_subscrib.insert(it->first);
			it++;
		}
	}
	lt_subscribe(_lt, tick_subscrib, _tick_callback);
}
void engine::clear_strategy()
{
	//策略不存在了那么订单和策略的映射关系也要清掉
	_estid_to_strategy.clear();
	_need_check_condition.clear();
	unsubscriber unsuber(*this);
	for (auto it : _strategy_map)
	{
		it.second->destroy(unsuber);
	}
	std::set<code_t> tick_unsubscrib;
	for (auto it = _tick_reference_count.begin();it != _tick_reference_count.end();)
	{
		if (it->second == 0)
		{
			tick_unsubscrib.insert(it->first);
			it = _tick_reference_count.erase(it);
		}
		else
		{
			it++ ;
		}
	}
	lt_unsubscribe(_lt, tick_unsubscrib);
	_strategy_map.clear();
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

const order_statistic& lt::engine::get_order_statistic(const code_t& code)const
{
	return lt_get_order_statistic(_lt,code);
}

estid_t engine::place_order(untid_t id,offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag)
{
	PROFILE_DEBUG(code.get_id());
	LOG_INFO("engine place order : ", code.get_id(), offset, direction, price, count);
	estid_t estid = lt_place_order(_lt, id, offset, direction, code, count,price, flag);
	if (estid != INVALID_ESTID)
	{
		regist_estid_strategy(estid, id);
	}
	return estid;
}

void engine::cancel_order(estid_t estid)
{
	LOG_DEBUG("cancel_order : ", estid);
	if (lt_cancel_order(_lt, estid))
	{
		remove_condition(estid);
	}
	else 
	{
		set_cancel_condition(estid, [](estid_t estid)->bool {
			return true;
		});
	}
}

const position_info& engine::get_position(const code_t& code) const
{
	return lt_get_position(_lt, code);
}

const order_info& engine::get_order(estid_t estid) const
{
	return lt_get_order(_lt, estid);
}

daytm_t engine::get_last_time() const
{
	return lt_get_last_time(_lt);
}

daytm_t engine::get_close_time()const
{
	if (_section_config == nullptr)
	{
		LOG_FATAL("section config not init");
		return 0;
	}
	return _section_config->get_close_time();
}

daytm_t engine::next_open_time(daytm_t time)const
{
	if (_section_config == nullptr)
	{
		LOG_FATAL("section config not init");
		return 0;
	}
	return _section_config->next_open_time(time);
}

bool engine::is_in_trading(daytm_t time)const
{
	if (_section_config == nullptr)
	{
		LOG_FATAL("section config not init");
		return false;
	}
	return _section_config->is_in_trading(time);
}

void engine::use_custom_chain(untid_t id,bool flag)
{
	lt_use_custom_chain(_lt, id, flag);
}

void engine::set_cancel_condition(estid_t estid, std::function<bool(estid_t)> callback)
{
	if(estid != INVALID_ESTID)
	{
		LOG_DEBUG("set_cancel_condition : ", estid);
		_need_check_condition[estid] = callback;
	}
}


void engine::check_condition()
{

	for (auto it = _need_check_condition.begin(); it != _need_check_condition.end();)
	{
		if (it->second(it->first))
		{
			if (lt_cancel_order(_lt,it->first)) 
			{
				it = _need_check_condition.erase(it);
			}
			else 
			{
				++it;
			}
		}
		else
		{
			++it;
		}
	}
}

void engine::remove_condition(estid_t estid) 
{
	auto odit = _need_check_condition.find(estid);
	if (odit != _need_check_condition.end())
	{
		_need_check_condition.erase(odit);
	}
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

void engine::bind_delayed_notify(std::shared_ptr<notify> notify)
{
	_all_notify.emplace_back(notify);
}

double_t engine::get_proximate_price(const code_t& code, double_t price)const
{
	if (_ps_config)
	{
		return _ps_config->get_proximate_price(code, price);
	}
	LOG_WARNING("_price_step_config null");
	return price;
}

deal_direction engine::get_deal_direction(const tick_info& prev, const tick_info& tick)const
{
	if (tick.price >= prev.sell_price() || tick.price >= tick.sell_price())
	{
		return deal_direction::DD_UP;
	}
	if (tick.price <= prev.buy_price() || tick.price <= tick.buy_price())
	{
		return deal_direction::DD_DOWN;
	}
	return deal_direction::DD_FLAT;
}

void engine::subscribe(const std::set<code_t>& tick_data, const std::map<code_t, std::set<uint32_t>>& bar_data)
{
	lt_subscribe(_lt, tick_data, _tick_callback);
	
}

void engine::unsubscribe(const std::set<code_t>& tick_data, const std::map<code_t, std::set<uint32_t>>& bar_data)
{
	
	for (auto& it : bar_data)
	{
		auto s = _bar_generator.find(it.first);
		if (s != _bar_generator.end())
		{
			for (auto& s_it : it.second)
			{
				auto a = s->second.find(s_it);
				if (a != s->second.end())
				{
					s->second.erase(a);
				}
			}
		}
	}
}