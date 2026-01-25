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
#include <basic_types.hpp>
#include "event_center.hpp"

namespace lt
{
	struct position_seed
	{
		code_t code;
		//总仓
		uint32_t total_long;
		uint32_t total_short;

		//昨仓
		uint32_t history_long;
		uint32_t history_short;

		position_seed() :total_long(0U), total_short(0U), history_long(0U), history_short(0) {}
	};

	typedef std::map<code_t, position_info> position_map;
	//
	typedef std::map<estid_t, order_info> entrust_map;

	struct account_info
	{
		double money;

		double frozen_monery;

		account_info() :
			money(.0F),
			frozen_monery(.0F)

		{}
	};
}
