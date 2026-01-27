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
#include "basic_define.h"
#include "basic_types.hpp"
#include "event_center.hpp"
namespace lt
{
	enum class market_event_type
	{
		MET_Invalid,
		MET_TickReceived,
	};
	/*
	 *	行情解析模块接口
	 */
	class market_api
	{
	public:

		virtual ~market_api() {}

	public:


		/*
		 *	订阅合约列表
		 */
		virtual void subscribe(const std::set<code_t>& codes) = 0;

		/*
		 *	退订合约列表
		 */
		virtual void unsubscribe(const std::set<code_t>& codes) = 0;

		/*
		*	逻辑更新
		*/
		virtual bool polling() = 0;

		/*
		*	绑定事件
		*/
		virtual void bind_event(market_event_type type, std::function<void(const std::vector<std::any>&)> handle) = 0;

		/*
		*	清理事件
		*/
		virtual void clear_event() = 0;

	};

	class actual_market : public market_api
	{

	public:

		virtual ~actual_market() {}

		/*
		*	初始化
		*/
		virtual bool login() = 0;

		/*
		*	注销
		*/
		virtual void logout() = 0;


	protected:

		std::unordered_map<std::string, std::string>& _id_excg_map;

		actual_market(std::unordered_map<std::string, std::string>& id_excg_map) :_id_excg_map(id_excg_map) {}
	};

	class sync_actual_market : public actual_market, public direct_event_source<market_event_type>
	{

	protected:

		sync_actual_market(std::unordered_map<std::string, std::string>& id_excg_map) :actual_market(id_excg_map) {}

		virtual void bind_event(market_event_type type, std::function<void(const std::vector<std::any>&)> handle) override
		{
			this->add_handle(type, handle);
		}

		virtual void clear_event()
		{
			this->clear_handle();
		}
	};

	class asyn_actual_market : public actual_market, public queue_event_source<market_event_type, 32768U>
	{

	protected:

		asyn_actual_market(std::unordered_map<std::string, std::string>& id_excg_map) :actual_market(id_excg_map) {}

		virtual bool polling()override
		{
			return queue_event_source<market_event_type, 32768U>::polling();
		}

		virtual void bind_event(market_event_type type, std::function<void(const std::vector<std::any>&)> handle) override
		{
			this->add_handle(type, handle);
		}

		virtual void clear_event()
		{
			this->clear_handle();
		}
	};

	class dummy_market : public market_api, public direct_event_source<market_event_type>
	{

	public:

		virtual ~dummy_market() {}

		/*
		*	绑定事件
		*/
		virtual void bind_event(market_event_type type, std::function<void(const std::vector<std::any>&)> handle) override
		{
			add_handle(type, handle);
		}

		virtual void clear_event()
		{
			this->clear_handle();
		}

	public:

		virtual void set_trading_range(uint32_t begin, uint32_t end) = 0;

		virtual void set_publish_callback(std::function<void(const std::vector<const lt::tick_info*>&)> publish_callback) = 0;

		virtual void set_crossday_callback(std::function<void(uint32_t form,uint32_t to)> crossday_callback) = 0;

		virtual void set_finish_callback(std::function<void()> finish_callback) = 0;

		virtual bool play() = 0;

		virtual void pause() = 0;

		virtual void resume() = 0;

		virtual bool is_finished() const = 0;
	};
}
