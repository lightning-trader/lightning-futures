// simulator.h: 目标的头文件。

#pragma once
#include <define.h>
#include <simulator.h>
#include <tick_loader.h>
#define BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/pool/object_pool.hpp>

struct order_match
{
	estid_t		est_id;
	uint32_t	queue_seat; //队列前面有多少个
	
	order_match():queue_seat(0)
	{}
};

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

	boost::lockfree::spsc_queue<const tick_info*, boost::lockfree::capacity<1024>>  _tick_queue;

	std::map<estid_t, order_info> _order_info;

	std::map<code_t, std::vector<order_match>> _order_match;

	//撮合时候用
	std::vector<const tick_info*> _current_tick_info;

	//上一帧的成交量，用于计算上一帧到这一帧成交了多少
	std::map<code_t,uint32_t> _last_frame_volume ;

	account_info _account_info;

	std::map<code_t, position_info> _position_info;



	double_t	_service_charge ;	//手续费
	uint32_t	_multiple ;			//资金倍数
	double_t	_margin_rate ;		//保证金率

public:

	tick_simulator():_loader(nullptr),
		_is_in_trading(false), 
		_current_trading_day(0), 
		_current_time(0),
		_current_tick(0),
		_current_index(0),
		_order_ref(0),
		_service_charge(0),
		_margin_rate(0),
		_multiple(0)
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
	virtual void update() override;

	virtual void set_trading_day(uint32_t tradeing_day) override 
	{
		_current_trading_day = tradeing_day;
	}

	virtual void play() override;

public:
	
	// md
	virtual void subscribe(const std::set<code_t>& codes)override;

	virtual void unsubscribe(const std::set<code_t>& codes)override;

	virtual time_t last_tick_time()const override;

	virtual void pop_tick_info(std::vector<const tick_info*>& result) override;
	
	virtual uint32_t get_trading_day()const override;
	
public:
	// td
	virtual bool is_usable()const override;

	virtual estid_t place_order(offset_type offset, direction_type direction, code_t code, uint32_t count, double_t price, order_flag flag) override;

	virtual void cancel_order(estid_t order_id) override;

	virtual const account_info* get_account() const override;

	virtual const position_info* get_position(code_t code) const override;

	virtual const order_info* get_order(estid_t order_id) const override;

	virtual void find_orders(std::vector<const order_info*>& order_result, std::function<bool(const order_info&)> func) const override;

	virtual void submit_settlement() override;

private:

	void load_data(code_t code,uint32_t trading_day);

	void publish_tick();

	void handle_order();

	estid_t make_estid();

	uint32_t get_front_count(code_t code, double_t price);

	void match_entrust(const tick_info* tick);

	void handle_entrust(const tick_info* tick, order_match& match, uint32_t max_volume);

	void handle_sell(const tick_info* tick, order_info& order, order_match& match, uint32_t deal_volume);
	
	void handle_buy(const tick_info* tick, order_info& order, order_match& match, uint32_t deal_volume);

	void handle_deal(order_info& order, uint32_t volume);
	
	void order_deal(order_info& order, uint32_t deal_volume);

	void order_cancel(estid_t order_id);

	boost::posix_time::ptime pt = boost::posix_time::ptime();

};
