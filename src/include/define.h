#pragma once
#include <stdio.h>
#include <string>
#include <set>
#include <map>
#include <array>
#include <vector>
#include <stdint.h>
#include <memory>

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
 
#define CODE_DATA_LEN 24
#define EXCG_OFFSET_LEN	8
#define EXCG_BEGIN_INDEX	CODE_DATA_LEN-EXCG_OFFSET_LEN
struct code_t
{
private:
	char _data[CODE_DATA_LEN];

public:

	code_t(const char* cd)
	{
		size_t ed = 0;
		memset(&_data, 0, sizeof(_data));
		
		for (size_t i=0; cd[i]!=NULL && i< CODE_DATA_LEN;i++)
		{
			char c = cd[i];
			if(c == '.')
			{
				ed = i;
				continue;
			}
			if(ed==0)
			{
				_data[i + EXCG_BEGIN_INDEX] = cd[i];
			}
			else
			{
				_data[i - ed - 1] = cd[i];
			}
		}
	}

	code_t()
	{
		memset(&_data, 0, sizeof(_data));
	}

	code_t(const code_t& obj)
	{
		memcpy_s(_data, CODE_DATA_LEN,obj._data, CODE_DATA_LEN);
	}
	code_t(const char* id, const char* excg_id)
	{
		memset(&_data, 0, sizeof(_data));
		strcpy_s(_data, strnlen_s(id, EXCG_BEGIN_INDEX - 1) + 1, id);
		strcpy_s(_data + EXCG_BEGIN_INDEX, strnlen_s(excg_id, EXCG_BEGIN_INDEX - 1)+1, excg_id);
	}

	bool operator < (const code_t& other)const
	{
		return memcmp(_data, other._data, CODE_DATA_LEN)<0;
	}
	bool operator == (const code_t& other)const
	{
		return memcmp(_data, other._data, CODE_DATA_LEN) == 0;
	}

	const char* get_id()const
	{
		return _data;
	}
	const char* get_excg()const
	{
		return _data + EXCG_BEGIN_INDEX;
	}
	bool is_excg(const char * excg)
	{
		if (strcmp(get_excg(), excg) != 0)
		{
			return false;
		}
		return true ;
	}
};

typedef uint32_t straid_t;

typedef uint64_t estid_t;

#define INVALID_ESTID 0x0LLU 

struct tick_info;

struct position_info;

struct account_info;

enum trading_optimal;

enum order_flag;

enum offset_type;

enum direction_type;

enum event_type;

enum error_type;

typedef enum log_level
{
	LLV_TRACE,
	LLV_DEBUG,
	LLV_INFO,
	LLV_WARNING,
	LLV_ERROR,
	LLV_FATAL
}log_level;

extern "C"
{
	EXPORT_FLAG void log_format(log_level lv,const char* format, ...);
}
#ifndef NDEBUG
#define LOG_TRACE(format, ...) log_format(LLV_TRACE,format, __VA_ARGS__);
#define LOG_DEBUG(format, ...) log_format(LLV_DEBUG,format, __VA_ARGS__);
#else
#define LOG_DEBUG(format, ...)
#define LOG_TRACE(format, ...)
#endif
#define LOG_INFO(format, ...) log_format(LLV_INFO,format, __VA_ARGS__);
#define LOG_WARNING(format, ...) log_format(LLV_WARNING,format, __VA_ARGS__);
#define LOG_ERROR(format, ...) log_format(LLV_ERROR,format, __VA_ARGS__);
#define LOG_FATAL(format, ...) log_format(LLV_FATAL,format, __VA_ARGS__);