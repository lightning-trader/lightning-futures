#pragma once
#include <define.h>
#include <strategy.h>

class strategy_manager
{
	friend strategy;

	ltobj& _lt;

	std::map<straid_t,std::shared_ptr<strategy>> _strategy_map;

	std::map<code_t,std::set<strategy*>> _code_to_strategy;

	std::map<estid_t, strategy*> _estid_to_strategy ;

public:

	strategy_manager(ltobj& lt):_lt(lt)
	{
		strategy_manager::_self = this ;
		lt_bind_callback(_lt
			, _tick_callback
			, _entrust_callback
			, _deal_callback
			, _trade_callback
			, _cancel_callback
			, _error_callback
			, _ready_callback
		);
	}

	inline ltobj& get_lt()
	{
		return _lt;
	};
private:

	static inline strategy_manager* _self;

	static inline void _tick_callback(const tick_info& tick)
	{
		if (_self)
		{
			auto it = _self->_code_to_strategy.find(tick.id);
			if (it == _self->_code_to_strategy.end())
			{
				return;
			}
			for (auto stra : it->second)
			{
				if (stra)
				{
					stra->on_tick(tick);
				}
			}

		}
	}

	static inline void _entrust_callback(const order_info& order)
	{
		if (_self)
		{
			auto it = _self->_estid_to_strategy.find(order.est_id);
			if (it == _self->_estid_to_strategy.end())
			{
				return;
			}
			if (it->second)
			{
				it->second->on_entrust(order);
			}
		}
	};

	static inline void _deal_callback(estid_t localid, uint32_t deal_volume, uint32_t total_volume)
	{
		if (_self)
		{
			auto it = _self->_estid_to_strategy.find(localid);
			if (it == _self->_estid_to_strategy.end())
			{
				return;
			}
			if (it->second)
			{
				it->second->on_deal(localid, deal_volume, total_volume);
			}
		}
	}

	static inline void _trade_callback(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
	{
		if (_self)
		{
			auto it = _self->_estid_to_strategy.find(localid);
			if (it == _self->_estid_to_strategy.end())
			{
				return;
			}
			if (it->second)
			{
				it->second->on_trade(localid, code, offset, direction, price, volume);
			}
			_self->unregist_estid_strategy(localid);
		}
	}

	static inline void _cancel_callback(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
	{
		if (_self)
		{
			auto it = _self->_estid_to_strategy.find(localid);
			if (it == _self->_estid_to_strategy.end())
			{
				return;
			}
			if (it->second)
			{
				it->second->on_cancel(localid, code, offset, direction, price, cancel_volume, total_volume);
			}
			_self->unregist_estid_strategy(localid);
		}
	}

	static inline void _error_callback(error_type type, estid_t localid, uint32_t error)
	{
		if (_self)
		{
			auto it = _self->_estid_to_strategy.find(localid);
			if (it == _self->_estid_to_strategy.end())
			{
				return;
			}
			if (it->second)
			{
				it->second->on_error(type,localid, error);
			}
			_self->unregist_estid_strategy(localid);
		}
	}

	static inline void _ready_callback()
	{
		if (_self)
		{
			for(auto& it : _self->_strategy_map)
			{
				it.second->on_ready();
			}
		}
	}

	static inline std::map<estid_t, std::function<bool(const tick_info&)>> _condition_function;
	static inline bool _condition_callback(estid_t localid, const tick_info& tick)
	{
		auto it = _condition_function.find(localid);
		if (it == _condition_function.end())
		{
			return false;
		}
		return it->second(tick);
	}

	
	
public:

	void regist_strategy(straid_t straid,std::shared_ptr<strategy> stra);

	void unregist_strategy(straid_t straid);

private:

	void regist_code_strategy(const code_t& code, strategy* stra);

	void unregist_code_strategy(const code_t& code, strategy* stra);

	void regist_estid_strategy(estid_t estid, strategy* stra);

	void unregist_estid_strategy(estid_t estid);
};