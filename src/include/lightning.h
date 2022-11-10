#include "define.h"
#include "data_types.hpp"
#include "context.h"
#include "driver/runtime_driver.h"
#include "driver/evaluate_driver.h"
#include "strategy.h"

#define LIGHTNING_VERSION 0.0.1

extern "C"
{
	EXPORT_FLAG runtime_driver* create_runtime_driver(const char* config_path);

	EXPORT_FLAG void destory_runtime_driver(runtime_driver* driver);

	EXPORT_FLAG evaluate_driver* create_evaluate_driver(const char* config_path);

	EXPORT_FLAG void destory_evaluate_driver(evaluate_driver* driver);

	EXPORT_FLAG context* create_context(driver* driver, strategy* strategy);

	EXPORT_FLAG void destory_context(context* ctx);
}
