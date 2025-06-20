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
#pragma once
#include <fstream>
#include <event_center.hpp>
#include <trading_context.h>
#include <data_channel.h>
#include <strategy.hpp>
#include <process_helper.hpp>
#include <inipp.h>

namespace lt::hft
{

	class engine : public queue_event_source<straid_t,1024>,public syringe
	{
	
	
	private: 

		void trading_begin()
		{
			this->_ctx->check_crossday();

			subscriber suber(_dc);
			for (auto it : _strategy_map)
			{
				it.second->init(suber);
			}

			suber.subscribe();
		}

		void trading_end()
		{
			unsubscriber unsuber(_dc);
			for (auto it : _strategy_map)
			{
				it.second->destroy(unsuber);
			}
			unsuber.unsubscribe();
		}

	public:
		
		/*
		* 发送消息
		*/
		inline void change_strategy(straid_t straid, bool openable, bool closeable, const std::string& param)
		{
			this->fire_event(straid, openable, closeable, param);
		}

		inline daytm_t get_last_time() const
		{
			return _ctx->get_last_time();
		}

		inline daytm_t last_order_time()const
		{
			return _ctx->last_order_time();
		}

		inline uint32_t get_trading_day()const
		{
			return _ctx->get_trading_day();
		}

		void set_trading_filter(filter_function callback)
		{
			_ctx->set_trading_filter(callback);
		}

		const order_statistic& get_order_statistic(const code_t& code)const
		{
			return _ctx->get_order_statistic(code);
		}

	protected:
		
		lt::trading_context* _ctx;

		lt::data_channel _dc;

		engine(const char* control_config)
			:
			_is_runing(false),
			_realtime_thread(nullptr),
			_bind_cpu_core(-1),
			_loop_interval(1000),
			_thread_priority(0),
			_ctx(nullptr),
			_dc(_ctx)
		{
			inipp::Ini<char> ini;
			std::ifstream is(control_config);
			ini.parse(is);
			auto it = ini.sections.find("control");
			if (it == ini.sections.end())
			{
				LTLOG_ERROR("runtime_engine init_from_file cant find [control]", control_config);
				return;
			}
			lt::params control_section(it->second);
			_bind_cpu_core = control_section.get<int16_t>("bind_cpu_core");
			_loop_interval = control_section.get<uint32_t>("loop_interval");
			_thread_priority = control_section.get<int16_t>("thread_priority");
			int16_t process_priority = control_section.get<int16_t>("process_priority");
			if (static_cast<int16_t>(PriorityLevel::LowPriority) <= process_priority && process_priority <= static_cast<int16_t>(PriorityLevel::RealtimePriority))
			{
				PriorityLevel level = static_cast<PriorityLevel>(process_priority);
				if (!process_helper::set_priority(level))
				{
					LTLOG_WARNING("set_priority failed");
				}
			}
		}
		virtual ~engine(){}

		/*启动*/
		bool start_service()
		{
			if (_is_runing)
			{
				LTLOG_WARNING("start a runing service");
				return false;
			}

			if (!_ctx->load_data())
			{
				LTLOG_ERROR("load data error");
				return false;
			}
			_is_runing = true;
			_realtime_thread = new std::thread([this]()->void {
				if (0 <= _bind_cpu_core && _bind_cpu_core < static_cast<int16_t>(std::thread::hardware_concurrency()))
				{
					if (!process_helper::thread_bind_core(static_cast<uint32_t>(_bind_cpu_core)))
					{
						LTLOG_WARNING("bind to core failed :", _bind_cpu_core);
					}
				}
				if (static_cast<int16_t>(PriorityLevel::LowPriority) <= _thread_priority && _thread_priority <= static_cast<int16_t>(PriorityLevel::RealtimePriority))
				{
					PriorityLevel level = static_cast<PriorityLevel>(_thread_priority);
					if (!process_helper::set_thread_priority(level))
					{
						LTLOG_WARNING("set_thread_priority failed");
					}
				}

				this->trading_begin();

				while (_is_runing/* || !_trader->is_idle()*/)
				{
					auto begin = std::chrono::system_clock::now();
					_ctx->update();
					_dc.update();
					this->process();
					for (auto& it : this->_strategy_map)
					{
						it.second->update();
					}
					auto use_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - begin);
					auto duration = std::chrono::microseconds(_loop_interval);
					if (use_time < duration)
					{
						std::this_thread::sleep_for(duration - use_time);
					}
				}
				this->trading_end();
				});
			return true;
		}

		/*停止*/
		bool stop_service()
		{
			if (!_is_runing)
			{
				LTLOG_WARNING("stop a not runing service");
				return false;
			}
			_is_runing = false;
			if (_realtime_thread)
			{
				LTLOG_INFO("destory realtime thread");
				_realtime_thread->join();
				LTLOG_INFO("realtime thread join end for delete");
				delete _realtime_thread;
				_realtime_thread = nullptr;
			}

			return true;
		}
		/***
		* 注册策略
		*/
		void regist_strategy(const std::vector <std::shared_ptr<lt::hft::strategy>>& strategys)
		{
			for (auto stra : strategys)
			{
				this->add_handle(stra->get_id(), std::bind(&lt::hft::strategy::handle_change, stra, std::placeholders::_1));
				_strategy_map[stra->get_id()] = stra;
			}
		}

		void clear_strategy()
		{
			this->clear_handle();
			//策略不存在了那么订单和策略的映射关系也要清掉
			_ctx->clear_condition();
			_strategy_map.clear();
		}


	private:

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
	
	public:
	
		virtual std::tuple<trading_context*, data_channel*> inject_data() override
		{
			return std::make_tuple(_ctx,&_dc);
		}
	
	protected:


		std::map<straid_t, std::shared_ptr<strategy>> _strategy_map;

		
		//实时的线程
		std::thread* _realtime_thread;

		int16_t _bind_cpu_core;

		int16_t _thread_priority;

		uint32_t _loop_interval;

		bool _is_runing;

	};
}
