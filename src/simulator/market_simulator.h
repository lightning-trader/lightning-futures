/*
Distributed under the MIT License(MIT)

Copyright(c) 2023 Jihua Zou EMail: ghuazo@qq.com QQ:137336521

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in the
Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
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

	market_simulator(const params& config)noexcept;
	
	virtual ~market_simulator()noexcept;
	

public:

	//simulator
	virtual void play(uint32_t trading_day, std::function<void(const tick_info&)> publish_callback) noexcept override;
	virtual bool is_finished() const noexcept override;

public:

	
	virtual void subscribe(const std::set<code_t>& codes)noexcept override;

	virtual void unsubscribe(const std::set<code_t>& codes)noexcept override;

	virtual void update() noexcept override;

private:

	void load_data()noexcept;

	void publish_tick()noexcept;

	void finish_publish()noexcept;

};
