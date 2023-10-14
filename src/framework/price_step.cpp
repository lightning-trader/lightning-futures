#include "price_step.h"
#include <data_types.hpp>
#include <rapidcsv.h>
#include "log_wapper.hpp"
#include <time_utils.hpp>

price_step::price_step(const std::string& config_path)
{
	LOG_INFO("price_step init ");
	_price_step_data.clear();
	rapidcsv::Document config_csv(config_path, rapidcsv::LabelParams(0, -1));
	for (size_t i = 0; i < config_csv.GetRowCount(); i++)
	{
		const std::string& code_str = config_csv.GetCell<std::string>("code", i);
		if(!code_str.empty())
		{
			double_t price_step = config_csv.GetCell<double_t>("price_step", i);
			_price_step_data[code_str.c_str()] = price_step;
		}
	}
}
price_step::~price_step()
{

}

double_t price_step::get_price_step(const code_t& code)const
{
	auto it = _price_step_data.find(code);
	if(it != _price_step_data.end())
	{
		return it->second;
	}
	return 1.0;
}

double_t price_step::get_proximate_price(const code_t& code, double_t row_price)const
{
	auto step = get_price_step(code);
	return std::round( row_price / step ) * step;
}