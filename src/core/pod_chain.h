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
#include <define.h>
#include <data_types.hpp>

/***  
* 
* 用户构造一个下单的责任链，实现开平互转等相关功能的拆分
* strategy 作为任务链的头节点，中间可以增加任意节点，exec_ctx 作为责任链的结束节点
* 实现不同的功能，只需要在中间增加节点即可
*/
class pod_chain
{

protected:
	
	pod_chain* _next ;
	
	class trader_api& _trader;
	class context& _ctx ;
	
public:
	pod_chain(context& ctx, pod_chain* next);
	virtual ~pod_chain();
	
	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) = 0;
	
};


class price_to_cancel_chain : public pod_chain
{
	
public:
	price_to_cancel_chain(context& ctx, pod_chain* next) :pod_chain(ctx, next)
	{}

	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;

};


//验证
class verify_chain : public pod_chain
{

public:
	
	verify_chain(context& ctx) :pod_chain(ctx, nullptr)
	{}


	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;


};