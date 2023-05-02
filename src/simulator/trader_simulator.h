#pragma once
#include <define.h>
#include <trader_api.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/pool/object_pool.hpp>
#include "order_container.h"
#include "contract_parser.h"
#include "position_container.h"


class trader_simulator : public dummy_trader
{

private:
	

	uint32_t _current_trading_day ;

	time_t _current_time;

	boost::lockfree::spsc_queue<tick_info, boost::lockfree::capacity<4>>  _pending_tick_info;
	
	uint32_t _order_ref;

	order_container _order_info;

	//撮合时候用
	std::vector<const tick_info*> _current_tick_info;

	//上一帧的成交量，用于计算上一帧到这一帧成交了多少
	std::map<code_t,uint32_t> _last_frame_volume ;

	account_info _account_info;
	
	position_container _position_info;

	std::atomic<bool> _is_submit_return ;

	uint32_t	_interval;			//间隔毫秒数
	
	contract_parser	_contract_parser;	//合约信息配置
	
public:

	trader_simulator():
		_current_trading_day(0), 
		_order_ref(0),
		_interval(1),
		_current_time(0),
		_is_submit_return(true)
	{}
	virtual ~trader_simulator()
	{
	}

	bool init(const boost::property_tree::ptree& config, trader_data& ret_data);

	

public:
	
	virtual void push_tick(const tick_info& tick) override;
	
	virtual void crossday(uint32_t trading_day) override;


public:

	virtual uint32_t get_trading_day()const override;
	// td
	virtual bool is_usable()const override;

	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;

	virtual void cancel_order(estid_t order_id) override;

	virtual void submit_settlement() override;


private:

	void handle_submit();

	estid_t make_estid();

	uint32_t get_front_count(const code_t& code, double_t price);

	void match_entrust(const tick_info* tick);

	void handle_entrust(const tick_info* tick, const order_match& match, uint32_t max_volume);

	void handle_sell(const tick_info* tick, const order_match& match, uint32_t deal_volume);
	
	void handle_buy(const tick_info* tick, const order_match& match, uint32_t deal_volume);

	void order_deal(order_info& order, uint32_t deal_volume, bool is_today);

	void order_error(error_type type, estid_t estid, uint32_t err);

	void order_cancel(const order_info& order, bool is_today);

	//冻结
	uint32_t frozen_deduction(estid_t est_id, const code_t& code, offset_type offset, direction_type direction, uint32_t count, double_t price, bool is_today);
	//解冻
	bool thawing_deduction(const code_t& code, offset_type offset, direction_type direction, uint32_t last_volume, double_t price, bool is_today);

};
