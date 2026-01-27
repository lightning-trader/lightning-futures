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
#include <list>
#include <receiver.h>
#include <basic_types.hpp>
#include <trading_context.h>
#include <data_wapper.hpp>

namespace lt
{
	/***
	*
	* bar 生成器
	*/
	class bar_generator
	{
	private:

		std::list<bar_info> _bar_cache;

	private:

		lt::code_t _code;

		uint32_t _period;

		bar_info _current_bar;
		
		double_t _detail_density;

		tick_info _prev_tick;

		trading_context*& _ctx;

		const data_wapper & _dw;

		std::set<lt::bar_receiver*> _bar_callback;

		seqtm_t _last_bar_end;


	public:

		bar_generator(const lt::code_t& code, uint32_t period, trading_context*& ctx, const data_wapper& dw);

		void load_history(size_t preload_bars = 128U);

		void clear_history();

		void insert_tick(const tick_info& tick);

		void add_receiver(lt::bar_receiver* receiver);

		void remove_receiver(lt::bar_receiver* receiver);

		const std::vector<bar_info> get_kline(size_t length);

		bool invalid()const;

		bool polling();

	private:

		void merge_into_bar(const tick_info& tick);

		void convert_to_bar(bar_info& bar,const ltd_bar_info& info);

		void create_new_bar();
	};

}
