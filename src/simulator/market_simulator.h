// simulator.h: 目标的头文件。

#pragma once
#include <define.h>
#include <market_api.h>
#include <tick_loader.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/pool/object_pool.hpp>


class market_simulator : public dummy_market
{

private:
	
	tick_loader* _loader ;

	std::set<code_t> _instrument_id_list ;

	uint32_t _current_trading_day ;

	std::vector<tick_info> _pending_tick_info ;

	time_t _current_time ;

	uint32_t _current_tick ;

	size_t _current_index ;

	boost::object_pool<tick_info> _tick_pool;

	uint32_t	_interval;			//间隔毫秒数
	
	bool _is_runing;
	
public:

	market_simulator():_loader(nullptr),
		_current_trading_day(0), 
		_current_time(0),
		_current_tick(0),
		_current_index(0),
		_interval(1),
		_is_runing(false)
	{}
	virtual ~market_simulator()
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
	virtual void play(uint32_t tradeing_day, std::function<void(const tick_info& info)> publish_callback) override;

public:
	
	// md
	virtual void subscribe(const std::set<code_t>& codes)override;

	virtual void unsubscribe(const std::set<code_t>& codes)override;

	
private:

	void load_data(const code_t& code,uint32_t trading_day);

	void publish_tick(std::function<void(const tick_info& info)> publish_callback);

};
