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
 
#define CODE_DATA_LEN 16

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
				_data[i] = cd[i];
			}
			else
			{
				_data[i-ed-1 + CODE_DATA_LEN / 2] = cd[i];
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
		strcpy_s(_data + CODE_DATA_LEN/2, CODE_DATA_LEN/2,id);
		strcpy_s(_data, CODE_DATA_LEN/2, excg_id);
	}

	bool operator < (const code_t& other)const
	{
		if (strcmp(get_excg(), other.get_excg())<0)
		{
			return true;
		}
		if (strcmp(get_id(), other.get_id())<0)
		{
			return true;
		}
		return false;
	}
	bool operator == (const code_t& other)const
	{
		if (strcmp(get_excg(), other.get_excg()) != 0)
		{
			return false;
		}
		if (strcmp(get_id(), other.get_id()) != 0)
		{
			return false;
		}
		return true;
	}

	const char* get_id()const
	{
		return _data + CODE_DATA_LEN / 2;
	}
	const char* get_excg()const
	{
		return _data ;
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

struct estid_t 
{
	std::string id ;

	estid_t(const char* id)
	{
		this->id = id;
	}

	estid_t(const estid_t& obj)
	{
		id = obj.id;
	}

	estid_t()
	{
		id = "";
	}

	const char* to_str()
	{
		return id.c_str();
	}

	bool is_valid()const
	{
		return !id.empty();
	}

	bool operator < (const estid_t& other)const
	{
		return id < other.id;
	}
	bool operator == (const estid_t& other)const
	{
		return id == other.id;
	}

	const estid_t& operator = (const estid_t& other)
	{
		this->id = other.id ;
		return *this;
	}
};


struct tick_info;

struct position_info;

struct account_info;

enum trading_optimal;

enum order_flag;

enum offset_type;

enum direction_type;

enum event_type;

