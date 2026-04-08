// psyche.h: 目标的头文件。

#pragma once
#include <vector>
#include <data_provider.h>
#include <library_helper.hpp>
#include <log_define.hpp>



class data_wapper
{
private: 

	dll_handle _handle;

	const void * _provider;

	typedef const void * (*initialize_function)(const char*, const char*, size_t, size_t);

	typedef void (*destroy_function)(const void*);

	typedef uint32_t (*get_version_code_function)();

	typedef size_t(*get_history_tick_function)(const void*, ltd_tick_info*, size_t, const char*, uint32_t);
	get_history_tick_function _get_history_tick;

	typedef size_t(*get_history_bar_function)(const void*, ltd_bar_info*, const char*, uint64_t, uint32_t, size_t);
	get_history_bar_function _get_history_bar;

	typedef size_t(*get_trading_calendar_function)(const void*, uint32_t*, size_t, uint32_t, uint32_t);
	get_trading_calendar_function _get_trading_calendar;

	typedef size_t(*get_all_instrument_function)(const void*, char(*)[32], size_t, uint32_t);
	get_all_instrument_function _get_all_instrument;

	typedef ltd_error_code(*get_instrument_info_function)(const void*, ltd_instrument_info*, const char*, uint32_t);
	get_instrument_info_function _get_instrument_info;

	typedef ltd_error_code(*get_last_error_function)();
	get_last_error_function _get_last_error;

public:

	data_wapper(const char* channel, const char* cache_path, size_t product_cache=2U, size_t kline_cache=16U)
	{
		_handle = library_helper::load_library("latf-data-v3xp");
		initialize_function initialize = (initialize_function)library_helper::get_symbol(_handle, "_initialize");
		_get_last_error = (get_last_error_function)library_helper::get_symbol(_handle, "_get_last_error");
		_get_history_tick = (get_history_tick_function)library_helper::get_symbol(_handle, "_get_history_tick");
		_get_history_bar = (get_history_bar_function)library_helper::get_symbol(_handle, "_get_history_bar");
		_get_trading_calendar = (get_trading_calendar_function)library_helper::get_symbol(_handle, "_get_trading_calendar");
		_get_all_instrument = (get_all_instrument_function)library_helper::get_symbol(_handle, "_get_all_instrument");
		_get_instrument_info = (get_instrument_info_function)library_helper::get_symbol(_handle, "_get_instrument_info");

		_provider = initialize(channel,cache_path, product_cache, kline_cache);
		if(_provider == NULL)
		{
			PRINT_FATAL("initialize error:",static_cast<uint32_t>(_get_last_error()));
		}
		else 
		{
			get_version_code_function get_version_code = (get_version_code_function)library_helper::get_symbol(_handle, "_get_version_code");
			PRINT_INFO("initialize success : ", get_version_code());
		}
	}

	virtual ~data_wapper()
	{
		destroy_function destroy = (destroy_function)library_helper::get_symbol(_handle, "_destroy");
		if(destroy)
		{
			destroy(_provider);
		}
		library_helper::free_library(_handle);
	}

	ltd_error_code get_history_tick(std::vector<ltd_tick_info>& result, const char* code, uint32_t trading_day)const
	{
		result.resize(72000U);
		size_t real_size = _get_history_tick(_provider,result.data(), result.size(), code, trading_day);
		result.resize(real_size);
		return _get_last_error();
	}
	
	ltd_error_code get_history_bar(std::vector<ltd_bar_info>& result, const char* code, ltd_period period, uint64_t target,size_t size)const
	{
		result.resize(size);
		size_t real_size = _get_history_bar(_provider, result.data(), code, target,static_cast<uint32_t>(period), size);
		result.resize(real_size);
		return _get_last_error();
	}

	ltd_error_code get_trading_calendar(std::vector<uint32_t>& result,uint32_t begin, uint32_t end)const
	{
		result.resize(3600U);
		size_t real_size = _get_trading_calendar(_provider, result.data(), result.size(), begin, end);
		result.resize(real_size);
		return _get_last_error();
	}
	ltd_error_code get_all_instrument(std::vector<std::string>& result, uint32_t trading_day)const
	{
		const size_t max_size = 20000U;
		char all_instrument[max_size][32]={0};
		
		size_t real_size = _get_all_instrument(_provider, all_instrument, max_size, trading_day);
		result.reserve(real_size);
		// 截断处理并设置实际长度
		for (size_t i = 0; i < real_size; i++) {
			result.emplace_back(all_instrument[i]);
		}
		return _get_last_error();
	}
	ltd_error_code get_instrument_info(ltd_instrument_info& result, const char* code, uint32_t trading_day)const
	{
		return _get_instrument_info(_provider, &result, code, trading_day);
	}
};