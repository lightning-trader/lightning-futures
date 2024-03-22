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
#include "define.h"
#include <engine_types.hpp>
#include <receiver.h>

namespace lt
{
	/***
	*
	* bar 生成器
	*/
	class bar_generator
	{

	private:

		uint32_t _period;

		bar_info _bar;

		uint32_t _minute;

		uint64_t _prev_volume;

		double_t _price_step;

		std::map<double_t, uint32_t> _poc_data;

		std::set<lt::bar_receiver*> _bar_callback;

	public:

		bar_generator(uint32_t period, double_t price_step) :_period(period), _price_step(price_step), _minute(0), _prev_volume(0) {}

		void insert_tick(const tick_info& tick);

		void add_receiver(lt::bar_receiver* receiver);

		void remove_receiver(lt::bar_receiver* receiver);

		bool invalid()const;
	};

}
