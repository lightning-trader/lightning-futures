#include "contract_parser.h"
#include <log_wapper.hpp>

contract_parser::contract_parser()
{

}
contract_parser::~contract_parser()
{
	
}

void contract_parser::init(const std::string& config_path)
{
	LOG_INFO("contract_parser init");
	_contract_info.clear();
	rapidcsv::Document config_csv(config_path, rapidcsv::LabelParams(0, -1));
	for (size_t i = 0; i < config_csv.GetRowCount(); i++)
	{
		contract_info info ;
		const std::string& code_str = config_csv.GetCell<std::string>("code", i);
		LOG_INFO("load contract code :", code_str.c_str());
		info.code = code_t(code_str.c_str());
		info.crge_type = static_cast<charge_type>(config_csv.GetCell<int32_t>("charge_type", i));
		info.open_charge = config_csv.GetCell<double_t>("open_charge", i);
		info.close_today_charge = config_csv.GetCell<double_t>("close_today_charge", i);
		info.close_yestoday_charge = config_csv.GetCell<double_t>("close_yestoday_charge", i);
	
		info.multiple = config_csv.GetCell<double_t>("multiple", i);
		info.margin_rate = config_csv.GetCell<double_t>("margin_rate", i);
		_contract_info[info.code] = info;
		
	}
}
const contract_info* contract_parser::get_contract_info(const code_t& code)const
{
	auto it = _contract_info.find(code);
	if(it != _contract_info.end())
	{
		return &(it->second);
 	}
	return nullptr;
}