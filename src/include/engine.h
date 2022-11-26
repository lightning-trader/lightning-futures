#pragma once
#include <define.h>
#include <lightning.h>
#include <functional>
#include <strategy.h>

class engine
{

private:

	static inline std::function<bool(const code_t& code, offset_type offset, direction_type direction, order_flag flag)> _filter_function = nullptr;
	static inline bool _filter_callback(const code_t& code, offset_type offset, direction_type direction, order_flag flag)
	{
		if (_filter_function)
		{
			return _filter_function(code, offset, direction, flag);
		}
		return true;
	}


public:

	/***
	* 增加策略
	*/
	void add_strategy(straid_t id, std::shared_ptr<strategy> stra);

	/**
	* opt:
	*	设置交易优化（开平互转）
	*	根据当前的持仓情况和优化方式将开多转成平空 开空转平多 或者 平多转开空 平空转开多
	* flag:
	*	如果触发一个下单信号，当前未成交委托单刚好有一个订单和这个订单对冲，
	*   则将当前下单信号转成对冲单的撤单信号；同时触发当前单的
	*/
	void set_trading_optimize(uint32_t max_position, trading_optimal opt = TO_INVALID, bool flag = false);


	/*
	* 设置交易过滤器
	*/
	void set_trading_filter(std::function<bool(const code_t& code, offset_type offset, direction_type direction, order_flag flag)> callback);

	/**
	* 获取当前交易日的订单统计
	*	跨交易日会被清空
	*/
	const order_statistic& get_order_statistic()const;

protected:

	ltobj _lt;

	std::unique_ptr<class strategy_manager> _strategy_manager ;
};


