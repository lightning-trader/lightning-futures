#pragma once
#include <define.h>
#include <event_center.hpp>



class exec_ctx
{

public:
	exec_ctx(class driver& driver,class strategy& strategy);
	virtual ~exec_ctx();

private:
	
	bool _is_runing ;

	class driver& _driver ;

	class market_api* _market;

	class trader_api* _trader;

	class strategy& _strategy;

private:

	void on_event_trigger(event_type type, const std::vector<std::any>& param);

public:
	
	void start() ;

	void stop();

private:

	void run();

};

