﻿#pragma once
#include <define.h>
#include <lightning.h>
#include <engine.h>

namespace lt
{
	class runtime_engine : public engine
	{

	public:

		runtime_engine(const char* config_path);
		virtual ~runtime_engine();

	public:

		void start_trading(const std::vector<std::shared_ptr<lt::strategy>>& strategys);

		void stop_trading();

	};
}


