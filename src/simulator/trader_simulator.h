#pragma once
#include <define.h>
#include <trader_api.h>
#include <params.hpp>
#include "order_container.h"
#include "contract_parser.h"
#include "position_container.h"


class trader_simulator : public dummy_trader
{

private:
	

	uint32_t _trading_day;

	daytm_t _current_time;
	
	uint32_t _order_ref;

	order_container _order_info;

	//撮合时候用
	std::vector<const tick_info*> _current_tick_info;

	//上一帧的成交量，用于计算上一帧到这一帧成交了多少
	std::map<code_t,uint64_t> _last_frame_volume ;

	account_info _account_info;
	
	position_container _position_info;

	uint32_t	_interval;			//间隔毫秒数
	
	contract_parser	_contract_parser;	//合约信息配置

public:

	trader_simulator(const params& config);
	
	virtual ~trader_simulator();

	
public:
	
	virtual void push_tick(const tick_info& tick) override;
	
	virtual void crossday(uint32_t trading_day) override;

	virtual const account_info& get_account() override
	{
		return _account_info;
	}

public:

	virtual uint32_t get_trading_day()const override;
	// td
	virtual bool is_usable()const override;

	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;

	virtual void cancel_order(estid_t order_id) override;

	virtual std::shared_ptr<trader_data> get_trader_data() override;

private:
	
	estid_t make_estid();

	uint32_t get_front_count(const code_t& code, double_t price);

	void match_entrust(const tick_info* tick);

	void handle_entrust(const tick_info* tick, const order_match& match, uint32_t max_volume);

	void handle_sell(const tick_info* tick, const order_match& match, uint32_t deal_volume);
	
	void handle_buy(const tick_info* tick, const order_match& match, uint32_t deal_volume);

	void order_deal(order_info& order, uint32_t deal_volume);

	void order_error(error_type type, estid_t estid, error_code err);

	void order_cancel(const order_info& order);

	//冻结
	error_code frozen_deduction(estid_t est_id, const code_t& code, offset_type offset, direction_type direction, uint32_t count, double_t price);
	//解冻
	bool thawing_deduction(const code_t& code, offset_type offset, direction_type direction, uint32_t last_volume, double_t price);

};
