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

#include "klineview_strategy.h"
#include "time_utils.hpp"
#include <string_helper.hpp>

using namespace lt;
using namespace lt::hft;


void klineview_strategy::on_init(subscriber& suber)
{
	suber.regist_bar_receiver(_code, _period,this,60);
	suber.regist_tick_receiver(_code, this);
}

void klineview_strategy::on_bar(const lt::bar_info& bar)
{
	PRINT_INFO(bar.id.to_string(), '\t', lt::seqtm_to_string(bar.time), '\t', bar.open, bar.high, bar.close, bar.low, '\t', bar.volume);
}

void klineview_strategy::on_tick(const lt::tick_info& tick) 
{
	auto& kline = get_kline(_code, _period, 5);
	for (auto& bar : kline)
	{
		PRINT_INFO(lt::seqtm_to_string(bar.time), '\t', bar.open, bar.high, bar.close, bar.low, '\t', bar.volume);
	}
}

void klineview_strategy::on_destroy(unsubscriber& unsuber)
{
	unsuber.unregist_bar_receiver(_code, _period, this);
	unsuber.unregist_tick_receiver(_code, this);
}
