#pragma once
#include <stdint.h>
#include <math.h>
#ifndef EXPORT_FLAG
#ifdef _MSC_VER
#	define EXPORT_FLAG __declspec(dllexport)
#else
#	define EXPORT_FLAG __attribute__((__visibility__("default")))
#endif
#endif

#ifndef PORTER_FLAG
#ifdef _MSC_VER
#	define PORTER_FLAG _cdecl
#else
#	define PORTER_FLAG 
#endif
#endif

#define MAX_CODE_LENGTH 32U

#define PRICE_VOLUME_LENGTH  5U

extern "C" 
{

	struct ltd_price_volume
	{
		double_t price;
		uint32_t volume;
	};

	struct ltd_tick_info
	{
		char code[MAX_CODE_LENGTH];

		uint32_t trading_day;

		uint32_t time;

		double_t price;

		uint32_t volume;

		double_t open;

		double_t close;

		double_t high;
		
		double_t low;

		double_t max;

		double_t min;

		double_t standard;

		double_t open_interest;

		double_t average_price;

		ltd_price_volume bid_order[PRICE_VOLUME_LENGTH];

		ltd_price_volume ask_order[PRICE_VOLUME_LENGTH];
	
	};

	EXPORT_FLAG const void* ltd_initialize(const char* cache_file_path,size_t lru_max_size);

	EXPORT_FLAG void ltd_destroy(const void* provider);

	EXPORT_FLAG size_t ltd_get_history_tick(const void* provider,ltd_tick_info* result,size_t max_size, const char* code, uint32_t trading_day);

	EXPORT_FLAG size_t ltd_get_daily_info(const void* provider, uint32_t* result, size_t max_size, const char* code, uint32_t begin, uint32_t end);
	
}
