#pragma once
#include <define.h>
#include <any>

class EXPORT_FLAG context
{

public:
	context(class driver& driver,class strategy& strategy);
	virtual ~context();

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

