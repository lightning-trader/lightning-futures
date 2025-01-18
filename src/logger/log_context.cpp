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
#include "log_context.h"
#include "nanolog.hpp"
#include <chrono>
#include <time_utils.hpp>
#include <string_helper.hpp>
#include <process_helper.hpp>

using namespace lt;
using namespace nanolog;

std::atomic < NanoLogger* > atomic_nanologger;

NanoLogger nanologger(atomic_nanologger,"./log",128U);

NanoLogLine* ltl_alloc_logline()
{
	return atomic_nanologger.load(std::memory_order_acquire)->alloc();
}

void ltl_recycle_logline(NanoLogLine* line)
{
	atomic_nanologger.load(std::memory_order_acquire)->recycle(line);
}

void ltl_dump_logline(NanoLogLine* line)
{
	atomic_nanologger.load(std::memory_order_acquire)->dump(line);
}

