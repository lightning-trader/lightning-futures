#include "lightning.h"

extern "C"
{
	EXPORT_FLAG runtime_driver* create_runtime_driver(const char* config_path)
	{
		auto dirver = new runtime_driver();
		if (!dirver->init_from_file(config_path))
		{
			delete dirver;
			dirver = nullptr ;
		}
		return dirver;
	}

	EXPORT_FLAG void destory_runtime_driver(runtime_driver* driver)
	{
		if(driver)
		{
			delete driver;
			driver = nullptr;
		}
	}

	EXPORT_FLAG evaluate_driver* create_evaluate_driver(const char* config_path)
	{
		auto dirver = new evaluate_driver();
		if (!dirver->init_from_file(config_path))
		{
			delete dirver;
			dirver = nullptr;
		}
		return dirver;
	}

	EXPORT_FLAG void destory_evaluate_driver(evaluate_driver* driver)
	{
		if (driver)
		{
			delete driver;
			driver = nullptr;
		}
	}

	EXPORT_FLAG context* create_context(driver* driver, strategy* strategy)
	{
		if(driver && strategy)
		{
			return new context(*driver,*strategy);
		}
		return nullptr;
	}

	EXPORT_FLAG void destory_context(context* context)
	{
		if (context)
		{
			delete context;
			context = nullptr;
		}
	}
}