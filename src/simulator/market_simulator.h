﻿// simulator.h: 目标的头文件。

#pragma once
#include <define.h>
#include <market_api.h>
#include <tick_loader.h>
#include <params.hpp>


class market_simulator : public dummy_market
{

	enum class execute_state
	{
		ES_Idle,
		ES_LoadingData,
		ES_PublishTick
	};

private:
	
	tick_loader* _loader ;

	std::set<code_t> _instrument_id_list ;

	uint32_t _current_trading_day ;

	std::vector<tick_info> _pending_tick_info ;

	std::function<void(const tick_info&)> _publish_callback;

	daytm_t _current_time ;

	size_t _current_index ;

	uint32_t	_interval;			//间隔毫秒数
	
	bool _is_finished;

	execute_state _state ;
	
public:

	market_simulator(const params& config);
	
	virtual ~market_simulator();
	

public:

	//simulator
	virtual void play(uint32_t trading_day, std::function<void(const tick_info&)> publish_callback) override;
	virtual bool is_finished() const override;

public:

	
	virtual void subscribe(const std::set<code_t>& codes)override;

	virtual void unsubscribe(const std::set<code_t>& codes)override;

	virtual void update() override;

private:

	void load_data();

	void publish_tick();

	void finish_publish();

};
