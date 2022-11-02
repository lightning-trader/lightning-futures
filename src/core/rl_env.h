#pragma once
#include <define.h>
#include <event_center.hpp>

class rl_env 
{

public:
	rl_env(class driver& driver);
	virtual ~rl_env();

private:

	bool _is_runing;

	class driver& _driver;

	class market_api* _market;

	class trader_api* _trader;

public:

	virtual void on_trigger(event_type type);

public:

	void step();


private:

	

};
