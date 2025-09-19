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
#include <stdio.h>
#include <string>
#include <set>
#include <map>
#include <any>
#include <array>
#include <vector>
#include <stdint.h>
#include <memory>
#include <functional>
#include <cstring>
#include <cmath>
#include <cstdarg>
#include <unordered_map>

#define EXCHANGE_ID_SHFE	"SHFE"	//上期所
#define EXCHANGE_ID_DCE		"DCE"	//大商所
#define EXCHANGE_ID_INE		"INE"	//能源中心
#define EXCHANGE_ID_CZCE	"CZCE"	//郑商所
#define EXCHANGE_ID_GFEX	"GFEX"	//广期所
#define EXCHANGE_ID_CFFEX	"CFFEX"	//中金所

#define INVALID_ESTID 0x0LLU

namespace lt{

	struct code_t;

	//日内时间（精度到毫秒）
	typedef uint32_t daytm_t;
	//有序的时间（日期:uint32_t|时间:daytm_t）
	typedef uint64_t seqtm_t;

	typedef uint64_t estid_t;

	struct tick_info;

	constexpr uint8_t TEI_OPEN_PRICE = 0;
	constexpr uint8_t TEI_CLOSE_PRICE = 1;
	constexpr uint8_t TEI_HIGH_PRICE = 2;
	constexpr uint8_t TEI_LOW_PRICE = 3;
	constexpr uint8_t TEI_MAX_PRICE = 4;
	constexpr uint8_t TEI_MIN_PRICE = 5;
	constexpr uint8_t TEI_STANDARD_PRICE = 6;
	typedef std::tuple<double_t, double_t, double_t, double_t, double_t, double_t, double_t> tick_extend;

	constexpr size_t PRICE_VOLUME_SIZE = 5U;
	typedef std::array<std::pair<double_t, uint32_t>, PRICE_VOLUME_SIZE> price_volume_array;

	typedef std::map<double_t, uint32_t> price_volume_map;

	enum class order_flag;

	enum class offset_type;

	enum class direction_type;

	enum class event_type;

	enum class error_type;

	enum class deal_status;

	enum class deal_direction;

	struct position_info;

	struct market_info;

	struct tape_info;

	struct bar_info;

}


