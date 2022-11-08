extern "C"
{
	EXPORT_FLAG actual_trader_api* create_trader_api(const boost::property_tree::ptree& config);
	
	EXPORT_FLAG void destory_trader_api(actual_trader_api* api);
}
