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
#include "engine.h"
#include "bar_generator.h"

using namespace lt;
using namespace lt::hft;

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
		_engine._bar_generator[code][period] = std::make_shared<bar_generator>(period, _engine._ctx.get_price_step(code));
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

engine::engine():_ctx(this)
{

}

engine::~engine()
{

}

void engine::on_init()
{
	subscriber suber(*this);
	for (auto it : _strategy_map)
	{
		it.second->init(suber);
	}
	std::set<code_t> tick_subscrib;
	for (auto it = _tick_reference_count.begin(); it != _tick_reference_count.end();)
	{
		if (it->second == 0)
		{
			it = _tick_reference_count.erase(it);
		}
		else
		{
			tick_subscrib.insert(it->first);
			it++;
		}
	}
	this->_ctx.subscribe(tick_subscrib, [this](const tick_info& tick)->void {
		auto tk_it = _tick_receiver.find(tick.id);
		if (tk_it != _tick_receiver.end())
		{
			for (auto tkrc : tk_it->second)
			{
				if (tkrc)
				{
					PROFILE_DEBUG(tick.id.get_id());
					tkrc->on_tick(tick);
					PROFILE_DEBUG(tick.id.get_id());
				}
			}
		}

		auto tp_it = _tape_receiver.find(tick.id);
		if (tp_it != _tape_receiver.end())
		{
			for (auto tprc : tp_it->second)
			{
				if (tprc)
				{
					lt::tape_info deal_info(tick.id, tick.time, tick.price);
					const auto& prev_tick = _ctx.get_previous_tick(tick.id);
					deal_info.volume_delta = static_cast<uint32_t>(tick.volume - prev_tick.volume);
					deal_info.interest_delta = tick.open_interest - prev_tick.open_interest;
					deal_info.direction = get_deal_direction(prev_tick, tick);
					tprc->on_tape(deal_info);
				}
			}
		}

		auto br_it = _bar_generator.find(tick.id);
		if (br_it != _bar_generator.end())
		{
			for (auto bg_it : br_it->second)
			{
				bg_it.second->insert_tick(tick);
			}
		}
	});
}

void engine::on_update()
{
	this->process();
	for (auto& it : this->_strategy_map)
	{
		it.second->update();
	}
}

void engine::on_destroy()
{
	unsubscriber unsuber(*this);
	for (auto it : _strategy_map)
	{
		it.second->destroy(unsuber);
	}
	std::set<code_t> tick_unsubscrib;
	for (auto it = _tick_reference_count.begin(); it != _tick_reference_count.end();)
	{
		if (it->second == 0)
		{
			tick_unsubscrib.insert(it->first);
			it = _tick_reference_count.erase(it);
		}
		else
		{
			it++;
		}
	}
	_ctx.unsubscribe(tick_unsubscrib);

}

void engine::regist_strategy(const std::vector<std::shared_ptr<lt::hft::strategy>>& strategies)
{
	subscriber suber(*this);
	for (auto it : strategies)
	{
		this->add_handle(it->get_id(),std::bind(&lt::hft::strategy::handle_change, &(*it), std::placeholders::_1));
		_strategy_map[it->get_id()] = (it);
	}
}
void engine::clear_strategy()
{
	this->clear_handle();
	//策略不存在了那么订单和策略的映射关系也要清掉
	_ctx.clear_condition();
	_strategy_map.clear();
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
