extern "C"
{
	EXPORT_FLAG actual_market_api* create_market_api(const boost::property_tree::ptree& config);

	EXPORT_FLAG void destory_market_api(actual_market_api* api);
}
