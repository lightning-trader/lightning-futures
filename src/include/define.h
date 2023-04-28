#pragma once
#include <stdio.h>
#include <string>
#include <set>
#include <map>
#include <array>
#include <vector>
#include <stdint.h>
#include <memory>
#include <functional>
#include <cstring>
#include <cmath>
#include <cstdarg>

#ifndef WIN32
#include "save_s.hpp"
#endif

#define ENUM_TYPE_INT  : int
#define ENUM_TYPE_CHAR  : char

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
#define EXCG_OFFSET_LEN	6
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
	bool operator != (const code_t& other)const
	{
		return memcmp(_data, other._data, CODE_DATA_LEN) != 0;
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

const code_t default_code;

typedef uint8_t untid_t;

#define MAX_UNITID 0xFFU 

typedef uint64_t estid_t;

#define INVALID_ESTID 0x0LLU

#define EXCHANGE_ID_SHFE	"SHFE"	//上期所
#define EXCHANGE_ID_DCE		"DCE"	//大商所
#define EXCHANGE_ID_CZCE	"CZCE"	//郑商所

struct tick_info;

struct deal_info;

struct position_info;

struct account_info;

enum trading_optimal ENUM_TYPE_INT;

enum order_flag ENUM_TYPE_CHAR;

enum offset_type ENUM_TYPE_CHAR;

enum direction_type ENUM_TYPE_CHAR;

enum event_type ENUM_TYPE_INT;

enum error_type ENUM_TYPE_INT;

enum deal_direction ENUM_TYPE_INT;

enum deal_status ENUM_TYPE_INT;

typedef std::function<bool(const code_t& code, offset_type offset, direction_type direction, uint32_t count, double_t price, order_flag flag)> filter_function;


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
#define LOG_TRACE(format, ...) log_format(LLV_TRACE, format, ##__VA_ARGS__);
#define LOG_DEBUG(format, ...) log_format(LLV_DEBUG, format, ##__VA_ARGS__);
#else
#define LOG_DEBUG(format, ...)
#define LOG_TRACE(format, ...)
#endif
#define LOG_INFO(format, ...) log_format(LLV_INFO, format, ##__VA_ARGS__);
#define LOG_WARNING(format, ...) log_format(LLV_WARNING, format, ##__VA_ARGS__);
#define LOG_ERROR(format, ...) log_format(LLV_ERROR, format, ##__VA_ARGS__);
#define LOG_FATAL(format, ...) log_format(LLV_FATAL, format, ##__VA_ARGS__);