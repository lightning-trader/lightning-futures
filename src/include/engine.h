#pragma once
#include <define.h>
#include <lightning.h>
#include <functional>
#include <strategy.h>


class engine
{

private:

	static inline std::function<bool(const code_t& code, offset_type offset, direction_type direction, double_t price, order_flag flag)> _filter_function = nullptr;
	static inline bool _filter_callback(const code_t& code, offset_type offset, direction_type direction, double_t price, order_flag flag)
	{
		if (_filter_function)
		{
			return _filter_function(code, offset, direction, price, flag);
		}
		return true;
	}


public:

	/***
	* 增加策略
	*/
	void add_strategy(straid_t id, std::shared_ptr<strategy> stra);


	/*
	* 设置交易过滤器
	*/
	void set_trading_filter(std::function<bool(const code_t& code, offset_type offset, direction_type direction, double_t price, order_flag flag)> callback);

	/**
	* 获取当前交易日的订单统计
	*	跨交易日会被清空
	*/
	const order_statistic& get_order_statistic()const;

	/**
	* 获取仓位信息
	*/
	const position_info& get_position(const code_t& code) const;

protected:

	ltobj _lt;

	std::unique_ptr<class strategy_manager> _strategy_manager ;
};


