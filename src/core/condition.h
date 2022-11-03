#pragma once
#include <data_types.hpp>

struct condition
{
	virtual bool check(const tick_info* tick) = 0;
};
//时间到达目标时间触发
struct time_out_cdt : condition
{
	time_out_cdt(time_t target_time):_target_time(target_time)
	{
	
	}
	virtual bool check(const tick_info* tick) override ;

private:
	time_t _target_time;
};

//价格上涨超过目标价格时候触发
struct price_up_cdt : condition
{
	price_up_cdt(double_t price) :_target_price(price)
	{

	}
	virtual bool check(const tick_info* tick) override;
	
private:
	double_t _target_price;
};

//价格跌穿目标价触发
struct price_down_cdt : condition
{
	price_down_cdt(double_t price) :_target_price(price)
	{

	}
	virtual bool check(const tick_info* tick) override;
	
private:
	double_t _target_price;
};

//冲高回落指定点位触发
struct fall_back_cdt : condition
{
	fall_back_cdt(double_t price, double_t fall_offset) :_highest_price(price), _fall_offset(fall_offset)
	{

	}
	virtual bool check(const tick_info* tick) override;

private:
	double_t _highest_price;
	double_t _fall_offset ;
};

//触底反弹指定点位触发
struct bounce_back_cdt : condition
{
	bounce_back_cdt(double_t price, double_t bounce_offset) :_lowest_price(price), _bounce_offset(bounce_offset)
	{

	}
	virtual bool check(const tick_info* tick) override;

private:
	double_t _lowest_price;
	double_t _bounce_offset;
};
