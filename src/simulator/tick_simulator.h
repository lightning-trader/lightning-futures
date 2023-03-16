// simulator.h: 目标的头文件。

#pragma once
#include <define.h>
#include <simulator.h>
#include <tick_loader.h>
#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/pool/object_pool.hpp>
#include "order_container.h"
#include "contract_parser.h"


class tick_simulator : public simulator
{

private:
	
	tick_loader* _loader ;

	std::set<code_t> _instrument_id_list ;

	uint32_t _current_trading_day ;

	std::vector<tick_info> _pending_tick_info ;

	time_t _current_time ;

	uint32_t _current_tick ;
	
	uint32_t _order_ref;

	size_t _current_index ;

	bool _is_in_trading ;

	boost::object_pool<tick_info> _tick_pool;

	order_container _order_info;

	//撮合时候用
	std::vector<const tick_info*> _current_tick_info;

	//上一帧的成交量，用于计算上一帧到这一帧成交了多少
	std::map<code_t,uint32_t> _last_frame_volume ;

	account_info _account_info;

	std::map<code_t, position_info> _position_info;

	std::atomic<bool> _is_submit_return ;

	uint32_t	_interval;			//间隔毫秒数
	
	contract_parser	_contract_parser;	//合约信息配置
	
	double_t	_compulsory_factor; //低于现有资金比例以后触发强平

public:

	tick_simulator():_loader(nullptr),
		_is_in_trading(false), 
		_current_trading_day(0), 
		_current_time(0),
		_current_tick(0),
		_current_index(0),
		_order_ref(0),
		_interval(1),
		_compulsory_factor(1),
		_is_submit_return(true)
	{}
	virtual ~tick_simulator()
	{
		if(_loader)
		{
			delete _loader ;
			_loader = nullptr ;
		}
	}

	bool init(const boost::property_tree::ptree& config);

public:

	//simulator

	virtual void play(uint32_t tradeing_day) override;

public:
	
	// md
	virtual void subscribe(const std::set<code_t>& codes)override;

	virtual void unsubscribe(const std::set<code_t>& codes)override;

	virtual time_t last_tick_time()const override;

	virtual uint32_t get_trading_day()const override;
	
public:
	// td
	virtual bool is_usable()const override;

	virtual estid_t place_order(offset_type offset, direction_type direction, const code_t& code, uint32_t count, double_t price, order_flag flag) override;

	virtual void cancel_order(estid_t order_id) override;

	virtual const account_info& get_account() const override;

	virtual const position_info& get_position(const code_t& code) const override;

	virtual uint32_t get_total_position() const override;

	virtual const order_info& get_order(estid_t order_id) const override;

	virtual void find_orders(std::vector<order_info>& order_result, std::function<bool(const order_info&)> func) const override;

	virtual void submit_settlement() override;

	virtual bool get_instrument(const code_t& code) override;

	virtual bool is_in_trading(const code_t& code) override;

private:

	void load_data(const code_t& code,uint32_t trading_day);

	void handle_submit();

	void publish_tick();

	void handle_order();

	void compulsory_closing();

	estid_t make_estid();

	uint32_t get_front_count(const code_t& code, double_t price);

	void match_entrust(const tick_info* tick);

	void handle_entrust(const tick_info* tick, const order_match& match, uint32_t max_volume);

	void handle_sell(const tick_info* tick, const order_match& match, uint32_t deal_volume);
	
	void handle_buy(const tick_info* tick, const order_match& match, uint32_t deal_volume);

	void order_deal(order_info& order, uint32_t deal_volume, bool is_today);

	void order_error(error_type type, estid_t estid, uint32_t err);

	void order_cancel(const order_info& order, bool is_today);

	boost::posix_time::ptime pt = boost::posix_time::ptime();

	//冻结
	uint32_t frozen_deduction(estid_t est_id, const code_t& code, offset_type offset, direction_type direction, uint32_t count, double_t price, bool is_today);
	//解冻
	bool thawing_deduction(const code_t& code, offset_type offset, direction_type direction, uint32_t last_volume, double_t price, bool is_today);

	void crossday_settlement();

};
