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
#include <basic_define.h>
#include <market_api.h>
#include <tick_loader.h>
#include <params.hpp>

namespace lt::driver
{
	class market_simulator : public dummy_market
	{


	private:

		tick_loader* _loader;

		std::set<code_t> _instrument_id_list;
		
		std::vector<uint32_t> _all_trading_day;
		
		size_t _current_day_index;

		std::vector<tick_detail> _pending_tick_info;

		std::function<void(const std::vector<const tick_info*>&)> _publish_callback;
		
		std::function<void(uint32_t form, uint32_t to)> _crossday_callback;
		
		std::function<void()> _finish_callback;

		daytm_t _current_time;

		size_t _current_index;

		uint32_t	_interval;			//间隔毫秒数

		std::atomic<bool> _is_finished;

		std::atomic<bool> _is_runing;

	public:

		market_simulator(const params& config);

		virtual ~market_simulator();


	public:

		virtual void set_trading_range(uint32_t begin,uint32_t end)override;

		virtual void set_publish_callback(std::function<void(const std::vector<const lt::tick_info*>&)> publish_callback)override;
		
		virtual void set_crossday_callback(std::function<void(uint32_t form, uint32_t to)> crossday_callback)override;
		
		virtual void set_finish_callback(std::function<void()> finish_callback)override;

		//simulator
		virtual bool play() override;

		virtual void pause() override;

		virtual void resume() override;

		virtual bool is_finished() const override;

	public:


		virtual void subscribe(const std::set<lt::code_t>& codes)override;

		virtual void unsubscribe(const std::set<lt::code_t>& codes)override;

		virtual bool polling() override;

	private:

		bool load_data();

		bool publish_tick();

		void finish_publish();

	};
}
