#pragma once
#include <define.h>
#include <data_types.hpp>
#include "./csv_recorder/csv_recorder.h"


extern "C"
{
	EXPORT_FLAG recorder* create_recorder(const boost::property_tree::ptree& config)
	{
		auto record_type = config.get<std::string>("record_type");
		if (record_type == "csv")
		{
			csv_recorder* api = new csv_recorder();
			//api->init(config);
			return api;
		}
		return nullptr;
	}

	EXPORT_FLAG void destory_recorder(csv_recorder* api)
	{
		if (nullptr != api)
		{
			delete api;
			api = nullptr;
		}
	}
}
