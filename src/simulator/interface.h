extern "C"
{
	EXPORT_FLAG simulator* create_simulator(const boost::property_tree::ptree& config);
	
	EXPORT_FLAG void destory_simulator(simulator* smlt);
	
}
