#include "sig_1_strategy.h"
#include "time_utils.hpp"
#include "signal/prb_signal.h"
#include "pms/pyramid_pms.h"

void sig_1_strategy::on_init()
{
	subscribe(_code);
	if(_expire != default_code)
	{
		subscribe(_expire);
	}
	use_custom_chain(TO_OPEN_TO_CLOSE, true);
	_order_data = static_cast<persist_data*>(get_userdata(sizeof(persist_data)));

	_current_signals.emplace_back(std::make_shared<prb_signal>(_prb_delta, [this](signal_type st)->void {
		_current_prb_st = st;
		}));

	_expire_signals.emplace_back(std::make_shared<prb_signal>(_prb_delta, [this](signal_type st)->void {
		_expire_prb_st = st;
		}));
}

void sig_1_strategy::on_ready()
{
	uint32_t trading_day = get_trading_day();
	_coming_to_close = make_datetime(trading_day, "14:58:00");
	if(_order_data->trading_day!=trading_day)
	{
		_order_data->trading_day = trading_day;
		_order_data->order_estid = INVALID_ESTID;
	}
	else
	{
		auto& order = get_order(_order_data->order_estid);
		if (order.est_id != INVALID_ESTID)
		{
			set_cancel_condition(order.est_id, [this](const tick_info& tick)->bool {

				if (tick.time > _coming_to_close)
				{
					return true;
				}
				return false;
				});
		}
		else
		{
			_order_data->order_estid = INVALID_ESTID;
		}
	}
	_current_pcm = std::make_shared<pyramid_pms>(get_position(_code), _open_once, _beta, TO_OPEN_TO_CLOSE);
	_expire_pcm = std::make_shared<pyramid_pms>(get_position(_expire), _open_once, _beta, TO_OPEN_TO_CLOSE);
}

void sig_1_strategy::on_tick(const tick_info& tick, const deal_info& deal)
{
	if (!is_trading_ready())
	{
		LOG_DEBUG("sig_1_strategy is_trading_ready not ready %s\n", tick.id.get_id());
		return;
	}
	if (tick.time > _coming_to_close)
	{
		//LOG_DEBUG("sig_1_strategy time > _coming_to_close %s %d %d\n", tick.id.get_id(), tick.time, _coming_to_close);
		return;
	}
	LOG_TRACE("on_tick time : %d.%d %s %f %llu %llu\n", tick.time,tick.tick,tick.id.get_id(), tick.price, _order_data->order_estids[CLOSE_LONG_ORDER], _order_data->order_estids[OPEN_LONG_ORDER]);
	
	if (tick.id == _expire)
	{
		const auto& pos = get_position(_expire);
		if(pos.empty())
		{
			return ;
		}
		for(auto it : _expire_signals)
		{
			it->tick(tick, deal);
		}
		if(pos.get_long_position() > 0 && _expire_prb_st == ST_SELL)
		{
			//卖出
			auto volume = _expire_pcm->get_sell_volume(OT_CLOSE);
			if(volume>0)
			{
				sell_for_close(_expire, volume);
			}
		}
		if (pos.get_short_position() > 0 && _expire_prb_st == ST_BUY)
		{
			//买入
			auto volume = _expire_pcm->get_buy_volume(OT_CLOSE);
			if (volume > 0)
			{
				buy_for_close(_expire, volume);
			}
		}
	}
	else
	{
		for (auto it : _current_signals)
		{
			it->tick(tick, deal);
		}
		if (_current_prb_st == ST_SELL)
		{
			//卖出
			auto volume = _current_pcm->get_sell_volume(OT_OPEN);
			if (volume > 0)
			{
				sell_for_open(_expire, volume);
			}
		}
		if (_current_prb_st == ST_BUY)
		{
			//买入
			auto volume = _current_pcm->get_buy_volume(OT_OPEN);
			if (volume > 0)
			{
				buy_for_open(_expire, volume);
			}
		}
	}
	
	
}



void sig_1_strategy::on_entrust(const order_info& order)
{
	LOG_INFO("emg_2_strategy on_entrust : %llu %s %d %d %f %d/%d\n", order.est_id, order.code,order.direction,order.offset,order.price, order.last_volume,order.total_volume);

	if (_order_data->order_estid == order.est_id)
	{
		set_cancel_condition(order.est_id, [this](const tick_info& tick)->bool {

			if (tick.time > _coming_to_close)
			{
				return true;
			}
			return false;
			});
	}
}

void sig_1_strategy::on_trade(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
{
	LOG_INFO("sig_1_strategy on_trade : %llu %s %d %d %f %d\n", localid, code, direction, offset, price, volume);
	if (localid == _order_data->order_estid)
	{
		_order_data->order_estid = INVALID_ESTID;
	}
	
}

void sig_1_strategy::on_cancel(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume,uint32_t total_volume)
{
	LOG_INFO("sig_1_strategy on_cancel : %llu %s %d %d %f %d\n", localid, code, direction, offset, price, cancel_volume);
	if (localid == _order_data->order_estid)
	{
		_order_data->order_estid = INVALID_ESTID;
	}
}

void sig_1_strategy::on_error(error_type type, estid_t localid, const uint32_t error)
{
	LOG_ERROR("emg_2_strategy on_error : %llu %d \n", localid, error);
	if(type != ET_PLACE_ORDER)
	{
		return ;
	}
	if (localid == _order_data->order_estid)
	{
		_order_data->order_estid = INVALID_ESTID;
	}
}
void sig_1_strategy::on_destory()
{
	unsubscribe(_code);
	if (_expire != default_code)
	{
		unsubscribe(_expire);
	}
}
