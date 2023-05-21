#pragma once
#include <define.h>
#include <lightning.h>
#include <receiver.h>
#include <strategy.h>
#include "notify.h"


namespace lt
{

	class engine;

	class subscriber
	{
	public:
		std::set<code_t> tick_subscrib;
		std::map<code_t, std::set<uint32_t>> bar_subscrib;
	private:
		std::map<code_t, std::set<tick_receiver*>>& _tick_receiver;
		std::map<code_t, std::map<uint32_t, bar_receiver*>>& _bar_receiver;
	public:
		subscriber(std::map<code_t, std::set<tick_receiver*>>& tick_recv, std::map<code_t, std::map<uint32_t, bar_receiver*>>& bar_recv) :
			_tick_receiver(tick_recv),
			_bar_receiver(bar_recv)
		{}

		void regist_tick_receiver(const code_t& code, tick_receiver* receiver);
		void regist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver);
	};
	class unsubscriber
	{
	public:
		std::set<code_t> tick_unsubscrib;
		std::map<code_t, std::set<uint32_t>> bar_unsubscrib;
	private:
		std::map<code_t, std::set<tick_receiver*>>& _tick_receiver;
		std::map<code_t, std::map<uint32_t, bar_receiver*>>& _bar_receiver;
	public:
		unsubscriber(std::map<code_t, std::set<tick_receiver*>>& tick_recv, std::map<code_t, std::map<uint32_t, bar_receiver*>>& bar_recv) :
			_tick_receiver(tick_recv),
			_bar_receiver(bar_recv)
		{}
		void unregist_tick_receiver(const code_t& code, tick_receiver* receiver);
		void unregist_bar_receiver(const code_t& code, uint32_t period);

	};

	class engine
	{
		friend subscriber;
		friend unsubscriber;

	private:

		static inline filter_function _filter_function = nullptr;
		static inline bool _filter_callback(const code_t& code, offset_type offset, direction_type direction, uint32_t count, double_t price, order_flag flag)
		{
			if (_filter_function)
			{
				return _filter_function(code, offset, direction, count, price, flag);
			}
			return true;
		}
	private:

		static inline engine* _self;

		static inline void _tick_callback(const tick_info& tick, const deal_info& deal)
		{
			if (_self)
			{
				auto it = _self->_tick_receiver.find(tick.id);
				if (it == _self->_tick_receiver.end())
				{
					return;
				}
				for (auto trc : it->second)
				{
					if (trc)
					{
						trc->on_tick(tick, deal);
					}
				}

			}
		}
		static inline void _bar_callback(uint32_t perid, const bar_info& bar)
		{
			if (_self)
			{
				auto it = _self->_bar_receiver.find(bar.id);
				if (it == _self->_bar_receiver.end())
				{
					return;
				}
				auto b_it = it->second.find(perid);
				if (b_it != it->second.end())
				{
					b_it->second->on_bar(bar);
				}
			}
		}

		static inline void _update_callback()
		{
			if (_self)
			{
				for(auto& it: _self->_strategy_map)
				{
					it.second->update();
				}
			}
		};

		static inline void _realtime_entrust_callback(const order_info& order)
		{
			if (_self)
			{
				auto it = _self->_estid_to_strategy.find(order.est_id);
				if (it == _self->_estid_to_strategy.end())
				{
					return;
				}
				auto stra = _self->get_strategy(it->second);
				if (stra)
				{
					stra->on_entrust(order);
				}
			}
		};

		static inline void _realtime_deal_callback(estid_t localid, uint32_t deal_volume, uint32_t total_volume)
		{
			if (_self)
			{
				auto it = _self->_estid_to_strategy.find(localid);
				if (it == _self->_estid_to_strategy.end())
				{
					return;
				}
				auto stra = _self->get_strategy(it->second);
				if (stra)
				{
					stra->on_deal(localid, deal_volume, total_volume);
				}
			}
		}

		static inline void _realtime_trade_callback(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
		{
			if (_self)
			{
				auto it = _self->_estid_to_strategy.find(localid);
				if (it == _self->_estid_to_strategy.end())
				{
					return;
				}
				auto stra = _self->get_strategy(it->second);
				if (stra)
				{
					stra->on_trade(localid, code, offset, direction, price, volume);
				}
				_self->unregist_estid_strategy(localid);
			}
		}

		static inline void _realtime_cancel_callback(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
		{
			if (_self)
			{
				auto it = _self->_estid_to_strategy.find(localid);
				if (it == _self->_estid_to_strategy.end())
				{
					return;
				}
				auto stra = _self->get_strategy(it->second);
				if (stra)
				{
					stra->on_cancel(localid, code, offset, direction, price, cancel_volume, total_volume);
				}
				_self->unregist_estid_strategy(localid);
			}
		}

		static inline void _realtime_error_callback(error_type type, estid_t localid, uint32_t error)
		{
			if (_self)
			{
				auto it = _self->_estid_to_strategy.find(localid);
				if (it == _self->_estid_to_strategy.end())
				{
					return;
				}
				auto stra = _self->get_strategy(it->second);
				if (stra)
				{
					stra->on_error(type, localid, error);
				}
				_self->unregist_estid_strategy(localid);
			}
		}

		static inline void _ready_callback()
		{
			if (_self)
			{
				for (auto& it : _self->_strategy_map)
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

		static inline void _delayed_entrust_callback(const order_info& order)
		{
			if (_self)
			{
				for(auto& it : _self->_all_notify)
				{
					it->on_entrust(order);
				}
			}
		};

		static inline void _delayed_deal_callback(estid_t localid, uint32_t deal_volume, uint32_t total_volume)
		{
			if (_self)
			{
				for (auto& it : _self->_all_notify)
				{
					it->on_deal(localid, deal_volume, total_volume);
				}
			}
		}

		static inline void _delayed_trade_callback(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
		{
			if (_self)
			{
				for (auto& it : _self->_all_notify)
				{
					it->on_trade(localid, code, offset, direction, price, volume);
				}
				
			}
		}

		static inline void _delayed_cancel_callback(estid_t localid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
		{
			if (_self)
			{
				for (auto& it : _self->_all_notify)
				{
					it->on_cancel(localid, code, offset, direction, price, cancel_volume, total_volume);
				}
			}
		}

		static inline void _delayed_error_callback(error_type type, estid_t localid, uint32_t error)
		{
			if (_self)
			{
				for (auto& it : _self->_all_notify)
				{
					it->on_error(type, localid, error);
				}
			}
		}

	private:


		void regist_estid_strategy(estid_t estid, straid_t straid);

		void unregist_estid_strategy(estid_t estid);

	public:

		engine(context_type ctx_type, const char* config_path);

		~engine();

		void init(const std::map<straid_t, std::shared_ptr<strategy>>& stra_map);

		void destory();

		/*
		* 设置交易过滤器
		*/
		void set_trading_filter(filter_function callback);

		/**
		* 获取当前交易日的订单统计
		*	跨交易日会被清空
		*/
		const order_statistic& get_order_statistic()const;


		/*
		*	下单单
		*	por_t 下单返回的id
		*/
		por_t place_order(untid_t id, offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price = 0, order_flag flag = order_flag::OF_NOR);
		/*
		 *	撤单
		 *	order_id 下单返回的id
		 */
		void cancel_order(estid_t order_id);

		/**
		* 获取仓位信息
		*/
		const position_info& get_position(const code_t& code) const;

		/**
		* 获取账户资金
		*/
		const account_info& get_account() const;

		/**
		* 获取委托订单
		**/
		const order_info& get_order(estid_t order_id) const;


		/**
		* 获取时间
		*
		*/
		time_t get_last_time() const;

		/**
		* 使用自定义交易通道
		*/
		void use_custom_chain(untid_t id, bool flag);

		/*
		* 设置撤销条件(返回true时候撤销)
		*/
		void set_cancel_condition(estid_t order_id, std::function<bool(const tick_info&)> callback);


		/**
		* 获取最后一次下单时间
		*	跨交易日返回0
		*/
		time_t last_order_time();

		/**
		* 获取用户数据，直接写入会被保存到共享内存中
		*	注意多个策略时候id不能改变
		*	ID最大值与localdb配置中的userdata_block对应
		*/

		void* get_userdata(untid_t id, size_t size);

		/**
		* 获取交易日
		*/
		uint32_t get_trading_day()const;

		/**
		*	是否准备就绪
		*/
		bool is_trading_ready()const;


		/*
		* 获取今日行情数据
		*/
		const today_market_info& get_today_market_info(const code_t& code)const;

		/*
		* 获取还未成交的开仓
		*/
		uint32_t get_pending_position(const code_t& code,offset_type offset, direction_type direction)const;

		/*
		* 绑定延时通知
		*/
		void bind_delayed_notify(std::shared_ptr<notify> notify);

	private:
		/**
		*	订阅行情
		*/
		void subscribe(const std::set<code_t>& tick_data, const std::map<code_t, std::set<uint32_t>>& bar_data);

		/**
		*	取消订阅行情
		*/
		void unsubscribe(const std::set<code_t>& tick_data, const std::map<code_t, std::set<uint32_t>>& bar_data);

		/**
		*	获取策略
		*/
		inline std::shared_ptr<strategy> get_strategy(straid_t id)const
		{
			auto it = _strategy_map.find(id);
			if (it == _strategy_map.end())
			{
				return nullptr;
			}
			return it->second;
		}


	protected:

		ltobj _lt;

		std::map<straid_t, std::shared_ptr<strategy>> _strategy_map;

		std::map<code_t, std::set<tick_receiver*>> _tick_receiver;

		std::map<code_t, std::map<uint32_t, bar_receiver*>> _bar_receiver;

		std::map<estid_t, straid_t> _estid_to_strategy;

		std::vector<std::shared_ptr<notify>> _all_notify;
	};
}
