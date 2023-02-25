#pragma once
#include <rapidcsv.h>
#include <define.h>
#include <data_types.hpp>

typedef enum 
{
	CT_FIXED_AMOUNT = 1 ,//固定金额
	CT_PRICE_RATIO = 2,//价格比例
}charge_type;

struct contract_info
{
	code_t code ;
	charge_type charge_type;
	double_t open_charge;
	double_t close_today_charge;
	double_t close_yestoday_charge;

	double_t multiple; //乘数
	double_t margin_rate; //保证金率


	contract_info():charge_type(CT_FIXED_AMOUNT), open_charge(.0F), close_today_charge(.0F), close_yestoday_charge(.0F), multiple(.0F), margin_rate(.0F) {}

	inline double_t get_service_charge(double_t price, offset_type offset,bool is_today)const
	{
		if(charge_type == CT_FIXED_AMOUNT)
		{
			if(offset == OT_OPEN)
			{
				return open_charge;
			}
			else
			{
				if(is_today)
				{
					return close_today_charge;
				}
				else
				{
					return close_yestoday_charge;
				}
			}
		}
		if (charge_type == CT_PRICE_RATIO)
		{
			if (offset == OT_OPEN)
			{
				return open_charge * price * multiple;
			}
			else
			{
				if (is_today)
				{
					return close_today_charge * price * multiple;
				}
				else
				{
					return close_yestoday_charge * price * multiple;
				}
			}
		}
		return .0F;
	}
};

class contract_parser
{

public:
	contract_parser();
	virtual ~contract_parser();

private:

	std::map<code_t, contract_info> _contract_info;
public:

	void init(const std::string& config_path);

	const contract_info* get_contract_info(const code_t&)const;

};

