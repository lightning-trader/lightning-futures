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
	
	class trader_api* _trader;
	class context* _ctx ;
	
public:
	pod_chain(context* ctx, pod_chain* next);
	virtual ~pod_chain();
	
	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) = 0;

protected:

	uint32_t get_open_pending()const;
	
};

class close_to_open_chain : public pod_chain
{
	//最优服务费（平转开）

public:
	
	close_to_open_chain(context* ctx,pod_chain* next) ;

	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;

};

class open_to_close_chain : public pod_chain
{
	//最优 保证金 （开转平）

public:
	open_to_close_chain(context* ctx, pod_chain* next) :pod_chain(ctx, next)
	{}

	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;

};

class price_to_cancel_chain : public pod_chain
{
	
public:
	price_to_cancel_chain(context* ctx, pod_chain* next) :pod_chain(ctx, next)
	{}

	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;

};
//移仓
class transfer_position_chain : public pod_chain
{

public:
	transfer_position_chain(context* ctx, pod_chain* next) :pod_chain(ctx, next)
	{}

	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;
};

//验证
class verify_chain : public pod_chain
{

public:
	
	verify_chain(context* ctx) :pod_chain(ctx, nullptr)
	{}


	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;


};