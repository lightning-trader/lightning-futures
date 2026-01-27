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
#include <trading_context.h>
#include <receiver.h>
#include <data_wapper.hpp>

namespace lt
{

	class data_channel;

	class subscriber
	{

	private:

		data_channel*& _dc;

		uint32_t _td;

	public:

		subscriber(data_channel*& chl,uint32_t td) :
			_dc(chl),_td(td)
		{}

		void regist_tick_receiver(const code_t& code, tick_receiver* receiver);
		void regist_tape_receiver(const code_t& code, tape_receiver* receiver);
		void regist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver);

		void subscribe();
		
	};

	class unsubscriber
	{

	private:
		data_channel*& _dc;
	public:
		unsubscriber(data_channel*& chl) :
			_dc(chl)
		{}
		void unregist_tick_receiver(const code_t& code, tick_receiver* receiver);
		void unregist_tape_receiver(const code_t& code, tape_receiver* receiver);
		void unregist_bar_receiver(const code_t& code, uint32_t period, bar_receiver* receiver);

		void unsubscribe();
		
	};

	class data_channel
	{

		friend subscriber;
		friend unsubscriber;

	public:
		
		data_channel(lt::trading_context*& ctx,const char* channel, const char* cache_path, size_t detail_cache_size = 8U, size_t bar_cache_size = 8U):_ctx(ctx), _dw(channel, cache_path, detail_cache_size, bar_cache_size) {}

		bool poll();

		const std::vector<bar_info> get_kline(const code_t& code, uint32_t period, size_t length)const;

		const std::vector<tick_info> get_ticks(const code_t& code, size_t length)const;

	private:

		void subscribe(uint32_t td);

		void unsubscribe();


	private:
		
		std::map<code_t, std::set<tick_receiver*>> _tick_receiver;

		std::map<code_t, std::vector<tick_info>> _tick_cache;

		std::map<code_t, std::set<tape_receiver*>> _tape_receiver;

		std::map<code_t, std::map<uint32_t, std::shared_ptr<class bar_generator>>> _bar_generator;

		std::map<code_t, uint32_t> _tick_reference_count;

		lt::trading_context*& _ctx;

		data_wapper _dw;

	};
	
}

