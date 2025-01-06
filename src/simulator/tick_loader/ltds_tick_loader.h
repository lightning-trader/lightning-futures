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

#include <tick_loader.h>
#include <dll_helper.hpp>
#include <data_provider.h>

namespace lt::driver
{
	class ltds_tick_loader : public tick_loader
	{
	private:

		typedef const void* (*ltd_initialize)(const char*, size_t);

		typedef size_t(*ltd_get_history_tick)(const void*, ltd_tick_info*, size_t, const char*, uint32_t);

		typedef void (*ltd_destroy)(const void*);

	public:
		
		ltds_tick_loader(const std::string& token, const std::string& cache_path, size_t lru_size);

		virtual ~ltds_tick_loader();

	public:

		virtual void load_tick(std::vector<tick_detail>& result, const code_t& code, uint32_t trade_day) override;

	private:

		dll_handle _handle;

		const void* _provider;

	};
}
