﻿/*
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
#include <tick_loader.h>
#include <basic_define.h>
#include <library_helper.hpp>
#include <data_wapper.hpp>

namespace lt::driver
{
	class ldts_tick_loader : public tick_loader
	{
	private:



	public:
		
		ldts_tick_loader(const std::string& channel, const std::string& cache_path, size_t detail_cache_size = 128U, size_t bar_cache_size = 819200U);

		virtual ~ldts_tick_loader();

	public:

		virtual void load_trading_day(std::vector<uint32_t>& result, uint32_t begin, uint32_t end) override;

		virtual void load_tick(std::vector<tick_detail>& result, const code_t& code, uint32_t trade_day) override;

	private:

		data_wapper _ltd;

	};
}
