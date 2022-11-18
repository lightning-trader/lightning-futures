#include <recorder.h>
#include "csv_recorder.h"

recorder* create_recorder(const boost::property_tree::ptree& config)
{
	auto type = config.get<std::string>("type");
	if (type == "csv")
	{
		csv_recorder* rcd = new csv_recorder();
		rcd->init(config);
		return rcd ;
	}
	return nullptr;
}

void destory_recorder(recorder* rcd)
{
	if(rcd != nullptr)
	{
		delete rcd ;
		rcd = nullptr;
	}
}
