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

#define MAX_SECTION_LENGTH 4U

#define WAITING_PRICE_LENGTH  5U

#define MAX_DETAIL_PRICE_SIZE  256U

extern "C" 
{

	typedef enum
	{
		EC_NO_ERROR,
		EC_SYSTEM_ERROR,		//系统错误
		EC_WEBAPI_ERROR,		//WEB ERROR
		EC_CONNECT_EXCEPTION,	//连接异常
		EC_SERVICE_ERROR,		//服务错误
		EC_LOAD_FILE_ERROR,		//加载文件错误
		EC_VECSION_MISMATCH,	//SDK 版本不匹配（升级到最新SDK）
		EC_ZSTD_ERROR,			//压缩解压错误
	}ltd_error_code;

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

		ltd_price_volume bid_order[WAITING_PRICE_LENGTH];

		ltd_price_volume ask_order[WAITING_PRICE_LENGTH];
	
	};

	typedef enum
	{
		BC_1S = 1,
		BC_2S = 2,
		BC_3S = 3,
		BC_5S = 5,
		BC_6S = 6,
		BC_10S = 10,
		BC_12S = 12,
		BC_15S = 15,
		BC_20S = 20,
		BC_30S = 30,
		BC_1M = 60,
		BC_2M = 120,
		BC_3M = 180,
		BC_5M = 300,
		BC_6M = 360,
		BC_10M = 600,
		BC_12M = 720,
		BC_15M = 900,
		BC_20M = 1200,
		BC_30M = 1800,
		BC_1H = 3600,
		BC_2H = 7200,
		BC_3H = 10800,
		BC_4H = 14400,
		BC_6H = 21600,
		BC_8H = 28800,
		BC_12H = 43200,
		BC_DAY = 86400,
		BC_WEEK = 604800,
		BC_MONTH = 2629746,//基于400年大周期的平均值计算获得
		BC_YEAR = 31556952,//基于400年大周期的平均值计算获得
		
	}ltd_period;

	struct ltd_bar_info
	{
		 //合约ID
		char code[MAX_CODE_LENGTH];

		uint64_t time; //时间(非标准时间戳)

		uint32_t period;

		double_t open;

		double_t close;

		double_t high;

		double_t low;

		uint32_t volume;
		
		double_t detail_density;
		//订单流中的明细
		size_t price_buy_size ;
		ltd_price_volume buy_volume[MAX_DETAIL_PRICE_SIZE];
		size_t price_sell_size;
		ltd_price_volume sell_volume[MAX_DETAIL_PRICE_SIZE];
		size_t price_other_size;
		ltd_price_volume other_volume[MAX_DETAIL_PRICE_SIZE];

		
	};

	struct ltd_section_info
	{
		uint32_t begin_daytm;
		uint32_t end_daytm;
	};

	struct ltd_instrument_info
	{
		//合约ID
		char code[MAX_CODE_LENGTH];

		char product_code[MAX_CODE_LENGTH];

		uint32_t trading_day = 0U;

		double_t price_step = .0;

		size_t section_size = 0U;

		ltd_section_info time_section[MAX_SECTION_LENGTH];


	};

	EXPORT_FLAG const void* _initialize(const char* channel, const char* cache_path, size_t product_cache_size, size_t kline_cache_size);

	EXPORT_FLAG void _destroy(const void* provider);

	EXPORT_FLAG ltd_error_code _get_last_error();

	EXPORT_FLAG uint32_t _get_version_code();

	EXPORT_FLAG size_t _get_history_tick(const void* provider,ltd_tick_info* result,size_t max_size, const char* code, uint32_t trading_day);

	EXPORT_FLAG size_t _get_history_bar(const void* provider, ltd_bar_info* result, const char* code, uint64_t target, uint32_t period, size_t size);

	EXPORT_FLAG size_t _get_trading_calendar(const void* provider, uint32_t* result, size_t max_size, uint32_t begin, uint32_t end);

	EXPORT_FLAG size_t _get_all_instrument(const void* provider, char(*result)[32], size_t max_size, uint32_t trading_day);

	EXPORT_FLAG ltd_error_code _get_instrument_info(const void* provider, ltd_instrument_info* result, const char* code, uint32_t trading_day);

}
