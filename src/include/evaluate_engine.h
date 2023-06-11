#pragma once
#include <define.h>
#include <lightning.h>
#include <strategy.h>
#include <engine.h>

namespace lt
{

	class evaluate_engine : public engine
	{

	public:

		evaluate_engine(const char* config_path);
		virtual ~evaluate_engine();

	public:

		void back_test(const std::vector<std::shared_ptr<lt::strategy>>& strategys, uint32_t trading_day);



	};


}
