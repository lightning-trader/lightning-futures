﻿#pragma once
#include "context.h"

class runtime : public context
{

	class actual_market* _market;

	class actual_trader* _trader;

public:

	runtime();
 	virtual ~runtime();
	
public:

	bool init_from_file(const std::string& config_path);

	bool login_account();

	void logout_account();

	virtual trader_api& get_trader() override;

	virtual market_api& get_market() override;

	virtual void on_update() override;

	virtual bool is_terminaled() override;


};