#pragma once
#include <define.h>
#include <data_types.hpp>
#include "event_center.hpp"

enum class trader_event_type : uint8_t
{
	TET_Invalid,
	TET_AccountChange,
	TET_PositionChange,
	TET_OrderCancel,
	TET_OrderPlace,
	TET_OrderDeal,
	TET_OrderTrade,
	TET_OrderError
};

struct trader_data
{
	account_info account;

	std::vector<order_info> orders;

	std::vector<position_info> positions;

};

typedef std::map<code_t, position_info> position_map;
//
typedef std::map<estid_t, order_info> entrust_map;

//下单接口管理接口
class trader_api
{
public:
	
	virtual ~trader_api(){}

public:

	/*
	*	初始化
	*/
	virtual void login() = 0;

	/*
	*	注销
	*/
	virtual void logout() = 0;

	/*
	 *	是否可用
	 */
	virtual bool is_usable()const = 0;

	/*
	 *	下单接口
	 *	entrust 下单的具体数据结构
	 */
	virtual estid_t place_order(offset_type offset, direction_type direction , const code_t& code, uint32_t count, double_t price , order_flag flag) = 0;

	/*
	 *	撤单
	 *	action	操作的具体数据结构
	 */
	virtual void cancel_order(estid_t order_id) = 0;

	/**
	* 获取当前交易日
	*/
	virtual uint32_t get_trading_day()const = 0;

	/**
	* 获取交易数据
	*/
	virtual std::shared_ptr<trader_data> get_trader_data() = 0;
};

class actual_trader : public trader_api , public event_source<trader_event_type, 128>
{

public:
	
	virtual ~actual_trader() {}

protected:

	std::shared_ptr<std::unordered_map<std::string, std::string>> _id_excg_map;

	actual_trader(const std::shared_ptr<std::unordered_map<std::string, std::string>>& id_excg_map) :_id_excg_map(id_excg_map) {}
};

class dummy_trader : public trader_api , public event_source<trader_event_type, 4>
{

public:

	virtual ~dummy_trader() {}

public:
	
	virtual void push_tick(const tick_info& tick) = 0;

	virtual void crossday(uint32_t trading_day) = 0;
};