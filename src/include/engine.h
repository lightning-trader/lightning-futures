#pragma once
#include <define.h>
#include <lightning.h>
#include <receiver.h>
#include <strategy.h>
#include "notify.h"
#include <log_wapper.hpp>
#include "../framework/price_step.h"
#include "../framework/bar_generator.h"

namespace lt
{

	class engine;

	class subscriber
	{
	
	private:
	
		engine& _engine;
		
	public:
	
		subscriber(engine& engine) :
			_engine(engine)
		{}

		void regist_tick_receiver(const code_t& code, tick_receiver* receiver);
		void regist_tape_receiver(const code_t& code, tape_receiver* receiver);
		void regist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver);

	};
	class unsubscriber
	{
	
	private:
		engine& _engine; 
	public:
		unsubscriber(engine& engine) :
			_engine(engine)
		{}
		void unregist_tick_receiver(const code_t& code, tick_receiver* receiver);
		void unregist_tape_receiver(const code_t& code, tape_receiver* receiver);
		void unregist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver);

	};

	class engine
	{
		friend subscriber;
		friend unsubscriber;
		friend strategy;
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

		static inline void _tick_callback(const tick_info& tick)
		{
			if (_self)
			{
				auto tk_it = _self->_tick_receiver.find(tick.id);
				if (tk_it != _self->_tick_receiver.end())
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
				
				auto tp_it = _self->_tape_receiver.find(tick.id);
				if (tp_it != _self->_tape_receiver.end())
				{
					for (auto tprc : tp_it->second)
					{
						if (tprc)
						{
							tape_info deal_info;
							const auto& prev_tick = lt_get_previous_tick(_self->_lt,tick.id);
							deal_info.volume_delta = static_cast<uint32_t>(tick.volume - prev_tick.volume);
							deal_info.interest_delta = tick.open_interest - prev_tick.open_interest;
							deal_info.direction = _self->get_deal_direction(prev_tick, tick);
							tprc->on_tape(deal_info);
						}
					}
				}
				
				auto br_it = _self->_bar_generator.find(tick.id);
				if (br_it != _self->_bar_generator.end())
				{
					for (auto bg_it : br_it->second)
					{
						bg_it.second->insert_tick(tick);
					}
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
				_self->check_condition();
			}
		};

		static inline void _entrust_callback(const order_info& order)
		{
			if (_self)
			{
				auto it = _self->_estid_to_strategy.find(order.estid);
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

		static inline void _deal_callback(estid_t estid, uint32_t deal_volume, uint32_t total_volume)
		{
			if (_self)
			{
				auto it = _self->_estid_to_strategy.find(estid);
				if (it == _self->_estid_to_strategy.end())
				{
					return;
				}
				auto stra = _self->get_strategy(it->second);
				if (stra)
				{
					stra->on_deal(estid, deal_volume, total_volume);
				}
			}
		}

		static inline void _trade_callback(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t volume)
		{
			if (_self)
			{
				auto it = _self->_estid_to_strategy.find(estid);
				if (it == _self->_estid_to_strategy.end())
				{
					return;
				}
				auto stra = _self->get_strategy(it->second);
				if (stra)
				{
					stra->on_trade(estid, code, offset, direction, price, volume);
				}
				_self->remove_condition(estid);
				_self->unregist_estid_strategy(estid);
			}
		}

		static inline void _cancel_callback(estid_t estid, const code_t& code, offset_type offset, direction_type direction, double_t price, uint32_t cancel_volume, uint32_t total_volume)
		{
			if (_self)
			{
				auto it = _self->_estid_to_strategy.find(estid);
				if (it == _self->_estid_to_strategy.end())
				{
					return;
				}
				auto stra = _self->get_strategy(it->second);
				if (stra)
				{
					stra->on_cancel(estid, code, offset, direction, price, cancel_volume, total_volume);
				}
				_self->remove_condition(estid);
				_self->unregist_estid_strategy(estid);
			}
		}

		static inline void _error_callback(error_type type, estid_t estid, error_code error)
		{
			if (_self)
			{
				auto it = _self->_estid_to_strategy.find(estid);
				if (it == _self->_estid_to_strategy.end())
				{
					return;
				}
				auto stra = _self->get_strategy(it->second);
				if (stra)
				{
					stra->on_error(type, estid, error);
				}
				if(type == error_type::ET_PLACE_ORDER)
				{
					_self->remove_condition(estid);
					_self->unregist_estid_strategy(estid);
				}
				else if (type == error_type::ET_CANCEL_ORDER)
				{
					_self->set_cancel_condition(estid, [](estid_t estid)->bool {
						return true;
						});
				}
				
			}
		}

		static inline void _init_callback()
		{
			if (_self)
			{
				subscriber suber(*_self);
				for (auto it : _self->_strategy_map)
				{
					it.second->init(suber);
				}
				std::set<code_t> tick_subscrib;
				for (auto it = _self->_tick_reference_count.begin(); it != _self->_tick_reference_count.end();)
				{
					if (it->second == 0)
					{
						it = _self->_tick_reference_count.erase(it);
					}
					else
					{
						tick_subscrib.insert(it->first);
						it++;
					}
				}
				lt_subscribe(_self->_lt, tick_subscrib, _tick_callback);
			}
		}

		static inline void _destroy_callback()
		{
			if (_self)
			{
				unsubscriber unsuber(*_self);
				for (auto it : _self->_strategy_map)
				{
					it.second->destroy(unsuber);
				}
				std::set<code_t> tick_unsubscrib;
				for (auto it = _self->_tick_reference_count.begin(); it != _self->_tick_reference_count.end();)
				{
					if (it->second == 0)
					{
						tick_unsubscrib.insert(it->first);
						it = _self->_tick_reference_count.erase(it);
					}
					else
					{
						it++;
					}
				}
				lt_unsubscribe(_self->_lt, tick_unsubscrib);
			}
		}

	protected:

		/***
		* 注册策略
		*/
		void regist_strategy(const std::vector<std::shared_ptr<lt::strategy>>& strategys);

		void clear_strategy();

		void check_condition();

		void remove_condition(estid_t estid);

	private:


		void regist_estid_strategy(estid_t estid, straid_t straid);

		void unregist_estid_strategy(estid_t estid);

	public:

		engine(context_type ctx_type, const char* config_path);

		~engine();

		/*
		* 设置交易过滤器
		*/
		void set_trading_filter(filter_function callback);

		/**
		* 获取当前交易日的订单统计
		*	跨交易日会被清空
		*/
		const order_statistic& get_order_statistic(const code_t& code)const;


		/*
		*	下单单
		*	estid 下单返回的id
		*/
		estid_t place_order(untid_t id, offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price = 0, order_flag flag = order_flag::OF_NOR);
		/*
		 *	撤单
		 *	estid 下单返回的id
		 */
		void cancel_order(estid_t estid);

		/**
		* 获取仓位信息
		*/
		const position_info& get_position(const code_t& code) const;

		/**
		* 获取委托订单
		**/
		const order_info& get_order(estid_t estid) const;


		/**
		* 获取时间
		*
		*/
		daytm_t get_last_time() const;

		/**
		* 获取收盘时间
		*
		*/
		daytm_t get_close_time() const;

		/**
		* 使用自定义交易通道
		*/
		void use_custom_chain(untid_t id, bool flag);

		/*
		* 设置撤销条件(返回true时候撤销)
		*/
		void set_cancel_condition(estid_t estid, std::function<bool(estid_t)> callback);


		/**
		* 获取最后一次下单时间
		*	跨交易日返回0
		*/
		daytm_t last_order_time();

		/**
		* 获取交易日
		*/
		uint32_t get_trading_day()const;

		/*
		* 获取今日行情数据
		*/
		const today_market_info& get_today_market_info(const code_t& code)const;

		
		/*
		* 绑定延时通知
		*/
		void bind_delayed_notify(std::shared_ptr<notify> notify);

		/*
		* 获取下单价格
		*/
		double_t get_proximate_price(const code_t& code,double_t price)const;


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

		/**
		* 获取交易方向
		*/
		deal_direction get_deal_direction(const tick_info& prev, const tick_info& tick)const;

	protected:

		ltobj _lt;

		std::map<straid_t, std::shared_ptr<strategy>> _strategy_map;

		std::map<code_t, std::set<tick_receiver*>> _tick_receiver;

		std::map<code_t, std::set<tape_receiver*>> _tape_receiver;

		std::map<code_t, std::map<uint32_t, std::shared_ptr<bar_generator>>> _bar_generator;

		std::map<code_t,uint32_t> _tick_reference_count ;

		std::map<estid_t, straid_t> _estid_to_strategy;

		std::vector<std::shared_ptr<notify>> _all_notify;

		std::shared_ptr<price_step> _ps_config;

		std::map<estid_t, std::function<bool(estid_t)>> _need_check_condition;
	};
}
