#include <recorder.h>
#include "csv_recorder.h"
#include "recorder_proxy.h"

recorder* create_recorder(const boost::property_tree::ptree& config)
{
	auto type = config.get<std::string>("type");
	if (type == "csv")
	{
		auto interval = config.get<uint32_t>("interval",100);
		csv_recorder* rcd = new csv_recorder();
		rcd->init(config);
		return new recorder_proxy(rcd, interval);
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
