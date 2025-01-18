// psyche.h: 目标的头文件。

#pragma once
#include <data_provider.h>
#include <vector>
#include <library_helper.hpp>

class ltd_wapper
{
private: 

	dll_handle _handle;

	const void * _provider;

	typedef const void * (*ltd_initialize)(const char*, size_t);

	typedef size_t(*ltd_get_history_tick)(const void*, ltd_tick_info*, size_t, const char*, uint32_t);

	typedef size_t (*ltd_get_daily_info)(const void*, uint32_t*, size_t, const char*, uint32_t, uint32_t);

	typedef void (*ltd_destroy)(const void*);

public:

	ltd_wapper(const char* cache_path, size_t lru_size = 128)
	{
		_handle = library_helper::load_library("liblt-data-v3xp");
		ltd_initialize initialize = (ltd_initialize)library_helper::get_symbol(_handle, "ltd_initialize");
		_provider = initialize(cache_path, lru_size);
	}

	virtual ~ltd_wapper()
	{
		ltd_destroy destroy = (ltd_destroy)library_helper::get_symbol(_handle, "ltd_destroy");
		destroy(_provider);
		library_helper::free_library(_handle);
	}

	void get_history_tick(std::vector<ltd_tick_info>& result, const char* code, uint32_t trading_day)
	{
		result.resize(72000U);
		ltd_get_history_tick get_history_tick = (ltd_get_history_tick)library_helper::get_symbol(_handle, "ltd_get_history_tick");
		size_t real_size = get_history_tick(_provider,result.data(), result.size(), code, trading_day);
		result.resize(real_size);
	}

	void get_daily_info(std::vector<uint32_t>& result,const char* code ,uint32_t begin, uint32_t end)
	{
		result.resize(3600U);
		ltd_get_daily_info get_daily_info = (ltd_get_daily_info)library_helper::get_symbol(_handle, "ltd_get_daily_info");
		size_t real_size = get_daily_info(_provider, result.data(), result.size(), code, begin, end);
		result.resize(real_size);
	}

};